param(
    [switch]$SkipBuild,
    [switch]$Preview
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
$verificationDirectory = Join-Path $workspace 'Temporary\ShaderLifecycleVerification'
$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
New-Item -ItemType Directory -Force -Path $verificationDirectory | Out-Null
if (-not $SkipBuild)
{
    & $msbuild (Join-Path $workspace 'MingEngine.sln') /restore /m /nologo /verbosity:minimal /property:Configuration=Debug /property:Platform=x64
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Debug build failed.'
    }
}

$executable = Join-Path $verificationDirectory 'ShaderLifecycleVerification.exe'
$testObject = Join-Path $verificationDirectory 'ShaderLifecycleVerification.obj'
$builtinObject = Join-Path $workspace 'Temporary\MingEngine_x64_Debug\BuiltinShaders.obj'
$compileCommand = 'call "{0}" >nul && cl /nologo /EHsc /std:c++17 /MDd /Zi /Od /D_DEBUG /I"{1}" /c "{2}" /Fo"{3}" /Fd"{4}" && link /nologo /debug /out:"{5}" "{3}" "{6}"' -f `
    $vcvars, (Join-Path $workspace 'Code'), (Join-Path $PSScriptRoot 'ShaderLifecycleVerification.cpp'), `
    $testObject, (Join-Path $verificationDirectory 'compile.pdb'), $executable, $builtinObject
& $env:ComSpec /d /c $compileCommand
if ($LASTEXITCODE -ne 0)
{
    throw 'Shader verification compile or link failed.'
}

& $executable $verificationDirectory
if ($LASTEXITCODE -ne 0)
{
    throw 'Shader lifecycle verification failed.'
}

$projectDirectory = Join-Path $verificationDirectory 'Project'
New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null
Copy-Item -Path (Join-Path $PSScriptRoot 'ShaderLifecycle\*.mshader') -Destination $projectDirectory -Force
[System.IO.File]::WriteAllText((Join-Path $projectDirectory 'project.ming'), '{"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn"}')
[System.IO.File]::WriteAllText((Join-Path $projectDirectory 'main.tscn'), "[ming_scene format=1]`n`n[node name=`"ShaderLifecycle`" type=`"Node3D`" parent=`"`"]`n")
$testObject = Join-Path $verificationDirectory 'ShaderLifecyclePreview.obj'
$executable = Join-Path $workspace 'Run\ShaderLifecyclePreview.exe'
$responsePath = Join-Path $verificationDirectory 'preview-link.rsp'
$objects = Get-ChildItem -LiteralPath (Join-Path $workspace 'Temporary\MingEngine_x64_Debug') -Filter '*.obj' |
    Where-Object { $_.Name -ne 'Main_Windows.obj' } |
    ForEach-Object { '"' + $_.FullName + '"' }
$linkArguments = @(
    '/NOLOGO', '/DEBUG', '/SUBSYSTEM:CONSOLE', '/MACHINE:X64', '/INCREMENTAL:NO',
    ('/OUT:"' + $executable + '"'),
    ('/PDB:"' + (Join-Path $verificationDirectory 'preview.pdb') + '"'),
    ('/IMPLIB:"' + (Join-Path $verificationDirectory 'preview.lib') + '"'),
    ('/LIBPATH:"' + (Join-Path $workspace 'Code') + '"'),
    ('/LIBPATH:"' + (Join-Path $workspace 'Code\ThirdParty\JoltPhysics') + '"'),
    'kernel32.lib', 'user32.lib', 'gdi32.lib', 'winspool.lib', 'comdlg32.lib',
    'advapi32.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib', 'uuid.lib',
    'odbc32.lib', 'odbccp32.lib', ('"' + $testObject + '"')
) + $objects
Set-Content -LiteralPath $responsePath -Value $linkArguments -Encoding UTF8
$compileCommand = 'call "{0}" >nul && cl /nologo /EHsc /std:c++17 /MDd /Zi /Od /D_DEBUG /I"{1}" /c "{2}" /Fo"{3}" /Fd"{4}" && link @"{5}"' -f `
    $vcvars, (Join-Path $workspace 'Code'), (Join-Path $PSScriptRoot 'ShaderLifecyclePreview.cpp'), `
    $testObject, (Join-Path $verificationDirectory 'preview-compile.pdb'), $responsePath
& $env:ComSpec /d /c $compileCommand
if ($LASTEXITCODE -ne 0)
{
    throw 'Shader engine preview compile or link failed.'
}

Push-Location (Join-Path $workspace 'Run')
try
{
    $previewArguments = @($projectDirectory)
    if ($Preview)
    {
        $previewArguments += '--preview'
    }
    & $executable @previewArguments
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Shader engine verification failed.'
    }
}
finally
{
    Pop-Location
}
