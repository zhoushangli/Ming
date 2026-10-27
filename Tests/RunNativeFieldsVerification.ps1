param([switch]$SkipBuild, [ValidateSet("NativeFieldsVerification", "NativeEditorStartupVerification")][string]$Suite = "NativeFieldsVerification")

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $repoRoot ('Temporary\' + $Suite)
$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

if (!$SkipBuild)
{
	& $msbuild (Join-Path $repoRoot 'MingEngine.sln') /m /nologo /verbosity:minimal /property:Configuration=Debug /property:Platform=x64
	if ($LASTEXITCODE -ne 0)
	{
		throw 'MingEngine Debug build failed.'
	}
}

$objects = Get-ChildItem -LiteralPath (Join-Path $repoRoot 'Temporary\MingEngine_x64_Debug') -Filter '*.obj' |
	Where-Object { $_.Name -ne 'Main_Windows.obj' }
$dependencies = ($objects.FullName -join ';') + ';%(AdditionalDependencies)'
$escape = [System.Security.SecurityElement]
$project = @'
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Debug|x64"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration></ItemGroup>
  <PropertyGroup Label="Globals"><WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><UseDebugLibraries>true</UseDebugLibraries><PlatformToolset>v143</PlatformToolset></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <PropertyGroup><OutDir>@OUTPUT@\</OutDir><IntDir>@OUTPUT@\obj\</IntDir></PropertyGroup>
  <ItemDefinitionGroup>
    <ClCompile><AdditionalIncludeDirectories>@CODE@</AdditionalIncludeDirectories><LanguageStandard>stdcpp17</LanguageStandard><RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary><ExceptionHandling>Sync</ExceptionHandling><PreprocessorDefinitions>_DEBUG;NOMINMAX</PreprocessorDefinitions></ClCompile>
    <Link><SubSystem>Console</SubSystem><GenerateDebugInformation>true</GenerateDebugInformation><AdditionalLibraryDirectories>@CODE@</AdditionalLibraryDirectories><AdditionalDependencies>@DEPENDENCIES@</AdditionalDependencies></Link>
  </ItemDefinitionGroup>
  <ItemGroup><ClCompile Include="@SOURCE@" /></ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
'@
$project = $project.Replace('@OUTPUT@', $escape::Escape($outputDirectory)).Replace('@CODE@', $escape::Escape((Join-Path $repoRoot 'Code')))
$project = $project.Replace('@SOURCE@', $escape::Escape((Join-Path $PSScriptRoot ($Suite + '.cpp')))).Replace('@DEPENDENCIES@', $escape::Escape($dependencies))
$projectPath = Join-Path $outputDirectory ($Suite + '.vcxproj')
Set-Content -LiteralPath $projectPath -Value $project -Encoding UTF8
& $msbuild $projectPath /m /nologo /verbosity:minimal /property:Configuration=Debug /property:Platform=x64
if ($LASTEXITCODE -ne 0)
{
	throw 'Native field verification build failed.'
}

Push-Location (Join-Path $repoRoot 'Run')
try
{
	if ($Suite -eq 'NativeEditorStartupVerification')
    {
        New-Item -ItemType Directory -Force -Path (Join-Path $outputDirectory 'MingSharp') | Out-Null
        Copy-Item -Path (Join-Path $repoRoot 'Run\MingSharp\*') -Destination (Join-Path $outputDirectory 'MingSharp') -Recurse -Force
        foreach ($mode in @('manager', 'editor', 'game'))
        {
            & (Join-Path $outputDirectory ($Suite + '.exe')) (Join-Path $repoRoot 'Temporary\NativeFieldsVerification\Project') $mode
            if ($LASTEXITCODE -ne 0)
            {
                throw ('Native startup verification failed: ' + $mode)
            }
        }
    }
    else
    {
        & (Join-Path $outputDirectory ($Suite + '.exe')) (Join-Path $outputDirectory 'Project')
    }
	if ($LASTEXITCODE -ne 0)
	{
		throw 'Native field verification failed.'
	}
}
finally
{
	Pop-Location
}
