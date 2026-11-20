param(
    [switch]$SkipBuild,
    [string[]]$Modules = @('--verify-arguments', '--verify-services-only', '--verify-project-manager', '--verify-game', '--verify-physics', '--verify-managed', '--verify-editor', '--verify-failed-enter', '--verify-failed-config')
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
$buildDirectory = Join-Path $workspace 'Temporary\MingEngine_x64_Debug'
$verificationDirectory = Join-Path $workspace 'Temporary\ApplicationLifecycleVerification'
$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
$vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
New-Item -ItemType Directory -Force -Path $verificationDirectory | Out-Null

if (-not $SkipBuild)
{
    & $msbuild (Join-Path $workspace 'MingEngine.sln') /m /nologo /verbosity:minimal /property:Configuration=Debug /property:Platform=x64
    if ($LASTEXITCODE -ne 0) { throw 'Debug build failed.' }
}

$managedProject = Join-Path $verificationDirectory 'ManagedProject'
New-Item -ItemType Directory -Force -Path $managedProject | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'ManagedLifecycleProbe.cs') -Destination $managedProject
[xml]$sdkPackage = Get-Content -LiteralPath (Join-Path $workspace 'Code/CSharp/MingSdkPackage.props')
$sdkIdentity = "$($sdkPackage.Project.PropertyGroup.MingSdkPackageId)/$($sdkPackage.Project.PropertyGroup.MingSdkPackageVersion)"
@"
<Project Sdk="$sdkIdentity">
  <PropertyGroup>
    <TargetFramework>net9.0</TargetFramework>
    <OutputType>Library</OutputType>
    <AssemblyName>Game</AssemblyName>
  </PropertyGroup>
</Project>
"@ | Set-Content -LiteralPath (Join-Path $managedProject 'Game.csproj') -Encoding UTF8
$mingApi = Join-Path $workspace 'Run\MingSharp\Api'
dotnet build (Join-Path $managedProject 'Game.csproj') --nologo --verbosity quiet "-p:MingApiDirectory=$mingApi"
if ($LASTEXITCODE -ne 0) { throw 'Managed lifecycle fixture build failed.' }

$testObject = Join-Path $verificationDirectory 'ApplicationLifecycleVerification.obj'
$executable = Join-Path $workspace 'Run\ApplicationLifecycleVerification.exe'
$responsePath = Join-Path $verificationDirectory 'link.rsp'
$objects = Get-ChildItem -LiteralPath $buildDirectory -Filter '*.obj' |
    Where-Object { $_.Name -ne 'Main_Windows.obj' } |
    ForEach-Object { '"' + $_.FullName + '"' }
$linkArguments = @(
    '/NOLOGO', '/DEBUG', '/SUBSYSTEM:CONSOLE', '/MACHINE:X64', '/INCREMENTAL:NO',
    ('/OUT:"' + $executable + '"'),
    ('/PDB:"' + (Join-Path $verificationDirectory 'ApplicationLifecycleVerification.pdb') + '"'),
    ('/IMPLIB:"' + (Join-Path $verificationDirectory 'ApplicationLifecycleVerification.lib') + '"'),
    ('/LIBPATH:"' + (Join-Path $workspace 'Code') + '"'),
    ('/LIBPATH:"' + (Join-Path $workspace 'Code\ThirdParty\JoltPhysics') + '"'),
    'kernel32.lib', 'user32.lib', 'gdi32.lib', 'winspool.lib', 'comdlg32.lib',
    'advapi32.lib', 'shell32.lib', 'ole32.lib', 'oleaut32.lib', 'uuid.lib',
    'odbc32.lib', 'odbccp32.lib', ('"' + $testObject + '"')
) + $objects
Set-Content -LiteralPath $responsePath -Value $linkArguments -Encoding UTF8
$compileCommand = 'call "{0}" >nul && cl /nologo /EHsc /std:c++17 /MDd /Zi /Od /D_DEBUG /I"{1}" /c "{2}" /Fo"{3}" /Fd"{4}" && link @"{5}"' -f `
    $vcvars, (Join-Path $workspace 'Code'), (Join-Path $PSScriptRoot 'ApplicationLifecycleVerification.cpp'), `
    $testObject, (Join-Path $verificationDirectory 'compile.pdb'), $responsePath
& $env:ComSpec /d /c $compileCommand
if ($LASTEXITCODE -ne 0) { throw 'Verification compile or link failed.' }

Push-Location (Join-Path $workspace 'Run')
try
{
    foreach ($module in $Modules)
    {
        Write-Output "Checking $module"
        & $executable $module
        if ($LASTEXITCODE -ne 0) { throw "Application lifecycle verification failed: $module" }
    }
}
finally
{
    Pop-Location
    Remove-Item -LiteralPath $executable -Force
}
