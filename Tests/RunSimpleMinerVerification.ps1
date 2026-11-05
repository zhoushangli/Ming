param([string]$ProjectPath = 'C:\Work\Ming\Project\SimpleMiner', [switch]$SkipBuild)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$projectRoot = (Resolve-Path -LiteralPath $ProjectPath).Path
if (!$SkipBuild)
{
    & (Join-Path $PSScriptRoot 'RunNativeFieldsVerification.ps1') -Suite CSharpWorkflowVerification
    if ($LASTEXITCODE -ne 0)
    {
        throw 'C# workflow verification failed.'
    }
}

$executable = Join-Path $repoRoot 'Temporary/CSharpWorkflowVerification/CSharpWorkflowVerification.exe'
if (!(Test-Path -LiteralPath $executable))
{
    throw 'Build CSharpWorkflowVerification before using -SkipBuild.'
}

& dotnet run --project (Join-Path $projectRoot 'Verification/Verification.csproj') -- $projectRoot
if ($LASTEXITCODE -ne 0)
{
    throw 'SimpleMiner data verification failed.'
}

$fixture = Join-Path $repoRoot ('Temporary/SimpleMinerRuntime/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $fixture | Out-Null
foreach ($file in @('Game.csproj', 'project.ming', 'Textures/BlockTiles.png', 'Data/GameConfig.xml', 'Data/Definitions/BlockSpriteSheet_BlockDefinitions.xml'))
{
    $target = Join-Path $fixture $file
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $target
}

$fixtureConfig = Get-Item -LiteralPath (Join-Path $fixture 'Data/GameConfig.xml')
$fixtureConfig.IsReadOnly = $false

$scenes = Join-Path $fixture 'Scenes'
New-Item -ItemType Directory -Force -Path $scenes | Out-Null
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Scenes') -Filter '*.tscn' -File | Copy-Item -Destination $scenes
$serviceScene = Get-Content -LiteralPath (Join-Path $scenes 'MinerEngine.tscn') -Raw | ConvertFrom-Json
$serviceScene.nodes[0].name = 'ServiceChecks'
$serviceScene.nodes[0].properties.PSObject.Properties.Remove('script')
$serviceScene | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $scenes 'ServiceChecks.tscn') -Encoding UTF8
$settings = Get-Content -LiteralPath (Join-Path $fixture 'project.ming') -Raw | ConvertFrom-Json
$settings.autoloads += [pscustomobject]@{ name = 'RuntimeChecks'; path = 'res://Scripts/SimpleMinerRuntimeChecks.cs' }
$settings | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $fixture 'project.ming') -Encoding UTF8

$scripts = Join-Path $fixture 'Scripts'
New-Item -ItemType Directory -Force -Path $scripts | Out-Null
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Scripts') -Filter '*.cs' -File | Copy-Item -Destination $scripts
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SimpleMinerRuntimeChecks.cs') -Destination $scripts
Set-Content -LiteralPath (Join-Path $scripts 'AutoloadResource.cs') -Encoding UTF8 -Value @'
using Ming;

public partial class AutoloadResource : MaterialResource
{
}
'@

& dotnet build (Join-Path $fixture 'Game.csproj') --nologo
if ($LASTEXITCODE -ne 0)
{
    throw 'SimpleMiner runtime fixture build failed.'
}

$previousPhase = $env:MING_SIMPLEMINER_PHASE
Push-Location (Join-Path $repoRoot 'Run')
try
{
    $env:MING_SIMPLEMINER_PHASE = 'editor'
    & $executable --simpleminer-editor $fixture
    if ($LASTEXITCODE -ne 0)
    {
        throw 'SimpleMiner editor Play/Stop save verification failed.'
    }

    foreach ($phase in @('first', 'second'))
    {
        $env:MING_SIMPLEMINER_PHASE = $phase
        & $executable --simpleminer $fixture
        if ($LASTEXITCODE -ne 0)
        {
            throw ('SimpleMiner runtime verification failed: ' + $phase)
        }
    }

    foreach ($case in @('missing-scene', 'non-node-script', 'duplicate-name'))
    {
        $failureProject = Join-Path $fixture ('.ming/Verification/Autoload-' + $case)
        New-Item -ItemType Directory -Force -Path (Join-Path $failureProject 'Scenes') | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $failureProject 'Scripts') | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $failureProject '.ming/dotnet/bin/Debug') | Out-Null
        Copy-Item -LiteralPath (Join-Path $fixture 'Game.csproj') -Destination $failureProject
        Copy-Item -LiteralPath (Join-Path $scripts 'AutoloadResource.cs') -Destination (Join-Path $failureProject 'Scripts')
        foreach ($file in @('Game.dll', 'Game.deps.json'))
        {
            Copy-Item -LiteralPath (Join-Path $fixture ('.ming/dotnet/bin/Debug/' + $file)) -Destination (Join-Path $failureProject '.ming/dotnet/bin/Debug')
        }

        Set-Content -LiteralPath (Join-Path $failureProject 'Scenes/Empty.tscn') -Encoding UTF8 -Value '{"nodes":[{"name":"Empty","type":"Node","parent":""}]}'
        $entries = @([pscustomobject]@{ name = 'Valid'; path = 'res://Scenes/Empty.tscn' })
        switch ($case)
        {
            'missing-scene' { $entries += [pscustomobject]@{ name = 'Missing'; path = 'res://Scenes/Missing.tscn' } }
            'non-node-script' { $entries += [pscustomobject]@{ name = 'Resource'; path = 'res://Scripts/AutoloadResource.cs' } }
            'duplicate-name' { $entries += [pscustomobject]@{ name = 'Valid'; path = 'res://Scenes/Empty.tscn' } }
        }

        [pscustomobject]@{ type = 'ProjectSettings'; version = 1; start_scene_path = 'res://Scenes/Empty.tscn'; autoloads = $entries } |
            ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $failureProject 'project.ming') -Encoding UTF8
        & $executable --autoload-failure $failureProject
        if ($LASTEXITCODE -ne 0)
        {
            throw ('AutoLoad failure cleanup failed: ' + $case)
        }
    }

    $env:MING_SIMPLEMINER_PHASE = 'pending'
    & $executable --autoload-pending-stop $fixture
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Shutdown with a pending Boot scene change failed.'
    }

    & $executable --resource-sample $fixture
    if ($LASTEXITCODE -ne 0)
    {
        throw 'Independent single-block fixture failed.'
    }
}
finally
{
    $env:MING_SIMPLEMINER_PHASE = $previousPhase
    Pop-Location
}

Write-Output ('SIMPLEMINER_VERIFICATION PASS fixture=' + $fixture)
