param(
    [switch]$SkipBuild,
    [switch]$Preview,
    [switch]$Capture,
    [switch]$EditorCapture,
    [float]$Scale = 1.25
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
$buildDirectory = Join-Path $workspace 'Temporary\MingEngine_x64_Debug'
$verificationDirectory = Join-Path $workspace 'Temporary\RoundedThemeVerification'
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

$testObject = Join-Path $verificationDirectory 'RoundedThemeVerification.obj'
$executable = Join-Path $workspace 'Run\RoundedThemeVerification.exe'
$responsePath = Join-Path $verificationDirectory 'link.rsp'
$objects = Get-ChildItem -LiteralPath $buildDirectory -Filter '*.obj' |
    Where-Object { $_.Name -notin @('Main_Windows.obj', 'Control.obj', 'RenderServer.obj') } |
    ForEach-Object { '"' + $_.FullName + '"' }
$linkArguments = @(
    '/NOLOGO', '/DEBUG', '/SUBSYSTEM:CONSOLE', '/MACHINE:X64', '/INCREMENTAL:NO',
    ('/OUT:"' + $executable + '"'),
    ('/PDB:"' + (Join-Path $verificationDirectory 'RoundedThemeVerification.pdb') + '"'),
    ('/IMPLIB:"' + (Join-Path $verificationDirectory 'RoundedThemeVerification.lib') + '"'),
    ('/LIBPATH:"' + (Join-Path $workspace 'Code') + '"'),
    ('/LIBPATH:"' + (Join-Path $workspace 'Code\ThirdParty\JoltPhysics') + '"'),
    'kernel32.lib', 'user32.lib', 'gdi32.lib', 'winspool.lib', 'comdlg32.lib',
    'advapi32.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib', 'uuid.lib',
    'odbc32.lib', 'odbccp32.lib', ('"' + $testObject + '"')
) + $objects
Set-Content -LiteralPath $responsePath -Value $linkArguments -Encoding UTF8
$compileCommand = 'call "{0}" >nul && cl /nologo /EHsc /std:c++17 /MDd /Zi /Od /D_DEBUG /I"{1}" /c "{2}" /Fo"{3}" /Fd"{4}" && link @"{5}"' -f `
    $vcvars, (Join-Path $workspace 'Code'), (Join-Path $PSScriptRoot 'RoundedThemeVerification.cpp'), `
    $testObject, (Join-Path $verificationDirectory 'compile.pdb'), $responsePath
& $env:ComSpec /d /c $compileCommand
if ($LASTEXITCODE -ne 0) { throw 'Verification compile or link failed.' }

Push-Location (Join-Path $workspace 'Run')
try
{
    if ($Preview)
    {
        & $executable --preview $Scale
    }
    elseif ($EditorCapture)
    {
        & $executable --editor-capture $Scale
    }
    elseif ($Capture)
    {
        & $executable --capture $Scale
    }
    else
    {
        & $executable
    }
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Rounded theme verification failed.'
    }
}
finally
{
    Pop-Location
}
