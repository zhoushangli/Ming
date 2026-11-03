param([string]$VsDevCmd = 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat')

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$build = Join-Path $repoRoot 'Temporary\GLFWOwnedWindow'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$archive = Join-Path $build 'glfw-3.4.zip'
if (!(Test-Path -LiteralPath $archive))
{
    Invoke-WebRequest 'https://github.com/glfw/glfw/archive/refs/tags/3.4.zip' -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne 'A133DDC3D3C66143EBA9035621DB8E0BCF34DBA1EE9514A9E23E96AFD39FD57A')
{
    throw 'GLFW 3.4 source archive checksum mismatch.'
}
Expand-Archive -LiteralPath $archive -DestinationPath $build -Force
$source = Join-Path $build 'glfw-3.4\src\win32_window.c'
$text = [IO.File]::ReadAllText($source).Replace("`r`n", "`n")
$before = @'
        window = GetPropW(handle, L"GLFW");
        if (window)
'@
$after = @'
        DWORD processID = 0;
        GetWindowThreadProcessId(handle, &processID);
        window = processID == GetCurrentProcessId() ? GetPropW(handle, L"GLFW") : NULL;
        if (window)
'@
if (!$text.Contains($before))
{
    throw 'Expected GLFW active-window lookup was not found.'
}
[IO.File]::WriteAllText($source, $text.Replace($before, $after), [Text.UTF8Encoding]::new($false))
$library = Join-Path $PSScriptRoot 'glfw3.lib'
$object = Join-Path $build 'win32_window.obj'
$base = Join-Path $build 'glfw-base.lib'
$patched = Join-Path $build 'glfw3.lib'
$members = & cmd /c "`"$VsDevCmd`" -arch=x64 -host_arch=x64 >nul && lib /nologo /list `"$library`""
$member = @($members | Where-Object { $_ -match '(^|[\/])win32_window\.obj$' })
if ($member.Count -ne 1)
{
    throw 'Expected exactly one Win32 window object in glfw3.lib.'
}
$include = Join-Path $build 'glfw-3.4\include'
$headers = Join-Path $build 'glfw-3.4\src'
& cmd /c "`"$VsDevCmd`" -arch=x64 -host_arch=x64 >nul && cl /nologo /c /MD /O2 /D_GLFW_WIN32 /I `"$include`" /I `"$headers`" /Fo`"$object`" `"$source`" && lib /nologo /remove:`"$($member[0])`" /out:`"$base`" `"$library`" && lib /nologo /out:`"$patched`" `"$base`" `"$object`""
if ($LASTEXITCODE -ne 0)
{
    throw 'GLFW Win32 patch build failed.'
}
Copy-Item -LiteralPath $patched -Destination $library -Force
