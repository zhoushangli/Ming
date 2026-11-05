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
foreach ($file in @('Game.csproj', 'project.ming', 'Scenes/Main.tscn', 'Scenes/BlockExample.tscn', 'Textures/BlockTiles.png', 'Data/GameConfig.xml', 'Data/Definitions/BlockSpriteSheet_BlockDefinitions.xml'))
{
    $target = Join-Path $fixture $file
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $target
}

$scripts = Join-Path $fixture 'Scripts'
New-Item -ItemType Directory -Force -Path $scripts | Out-Null
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Scripts') -Filter '*.cs' -File | Copy-Item -Destination $scripts
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SimpleMinerRuntimeChecks.cs') -Destination $scripts
$sceneScript = Join-Path $scripts 'WorldScene.cs'
$source = Get-Content -LiteralPath $sceneScript -Raw
$source = $source.Replace('NativeFuncs.Log("SimpleMiner ready:', 'SimpleMinerRuntimeChecks.Ready(this, camera);' + [Environment]::NewLine + '        NativeFuncs.Log("SimpleMiner ready:')
$source = $source.Replace('world.Update(camera.GetLocalPosition());', 'world.Update(camera.GetLocalPosition());' + [Environment]::NewLine + '        SimpleMinerRuntimeChecks.Tick(this);')
Set-Content -LiteralPath $sceneScript -Value $source -Encoding UTF8

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
