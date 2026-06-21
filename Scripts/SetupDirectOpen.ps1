[CmdletBinding()]
param(
    [string]$EngineRoot,
    [switch]$Open
)

$ErrorActionPreference = 'Stop'

function Get-ProjectRoot {
    $scriptDir = Split-Path -Parent $MyInvocation.ScriptName
    return (Resolve-Path (Join-Path $scriptDir '..')).Path
}

function Resolve-EngineRootCandidate {
    param([string]$Candidate)

    if ([string]::IsNullOrWhiteSpace($Candidate)) {
        return $null
    }

    $resolved = Resolve-Path -LiteralPath $Candidate -ErrorAction SilentlyContinue
    if (-not $resolved) {
        return $null
    }

    $path = $resolved.Path
    if ((Split-Path -Leaf $path) -ieq 'Engine' -and (Test-Path -LiteralPath (Join-Path $path 'Binaries\Win64\UnrealEditor.exe'))) {
        return (Split-Path -Parent $path)
    }

    if (Test-Path -LiteralPath (Join-Path $path 'Engine\Binaries\Win64\UnrealEditor.exe')) {
        return $path
    }

    return $null
}

function Get-EngineVersion {
    param([string]$Root)

    $versionPath = Join-Path $Root 'Engine\Build\Build.version'
    if (-not (Test-Path -LiteralPath $versionPath)) {
        return $null
    }

    return Get-Content -LiteralPath $versionPath -Raw | ConvertFrom-Json
}

function Find-EngineRoot {
    param([string]$RequestedRoot)

    $candidates = New-Object System.Collections.Generic.List[string]

    if ($RequestedRoot) {
        $candidates.Add($RequestedRoot)
    }

    foreach ($envName in @('UE_5_8_ROOT', 'UE_ROOT', 'UNREAL_ENGINE_ROOT')) {
        $value = [Environment]::GetEnvironmentVariable($envName)
        if ($value) {
            $candidates.Add($value)
        }
    }

    $regPath = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    if (Test-Path -LiteralPath $regPath) {
        $props = Get-ItemProperty -LiteralPath $regPath
        foreach ($prop in $props.PSObject.Properties) {
            if ($prop.Name -notlike 'PS*' -and $prop.Value -is [string]) {
                $candidates.Add($prop.Value)
            }
        }
    }

    foreach ($path in @(
        'C:\Program Files\Epic Games\UE_5.8',
        'D:\Program Files\Epic Games\UE_5.8',
        'E:\Program Files\Epic Games\UE_5.8',
        'E:\soft_UE\UE\UE_5.8',
        'E:\UE\SourceEngine'
    )) {
        $candidates.Add($path)
    }

    $validRoots = @()
    foreach ($candidate in $candidates) {
        $root = Resolve-EngineRootCandidate -Candidate $candidate
        if ($root -and ($validRoots -notcontains $root)) {
            $validRoots += $root
        }
    }

    if ($validRoots.Count -eq 0) {
        throw 'Unreal Engine root was not found. Pass -EngineRoot with your UE 5.8 install path.'
    }

    $ue58Roots = @()
    foreach ($root in $validRoots) {
        $version = Get-EngineVersion -Root $root
        if ($version -and [int]$version.MajorVersion -eq 5 -and [int]$version.MinorVersion -eq 8) {
            $ue58Roots += $root
        }
    }

    if ($ue58Roots.Count -gt 0) {
        return $ue58Roots[0]
    }

    Write-Warning "No UE 5.8 root was found. Using first valid Unreal Engine root: $($validRoots[0])"
    return $validRoots[0]
}

function Assert-RequiredFile {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required prebuilt binary: $RelativePath"
    }
}

$projectRoot = Get-ProjectRoot
$uprojectPath = Join-Path $projectRoot 'ToyTowerDefense.uproject'
if (-not (Test-Path -LiteralPath $uprojectPath)) {
    throw "Project file not found: $uprojectPath"
}

$projectJson = Get-Content -LiteralPath $uprojectPath -Raw | ConvertFrom-Json
$engineAssociation = [string]$projectJson.EngineAssociation
if ([string]::IsNullOrWhiteSpace($engineAssociation)) {
    throw 'ToyTowerDefense.uproject does not contain EngineAssociation.'
}

$resolvedEngineRoot = Find-EngineRoot -RequestedRoot $EngineRoot
$editorExe = Join-Path $resolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'

$requiredFiles = @(
    'Binaries\Win64\ToyTowerDefenseEditor.target',
    'Binaries\Win64\UnrealEditor-ToyTowerDefense.dll',
    'Binaries\Win64\UnrealEditor.modules',
    'Plugins\GameplayMessageRouter\Binaries\Win64\UnrealEditor-GameplayMessageNodes.dll',
    'Plugins\GameplayMessageRouter\Binaries\Win64\UnrealEditor-GameplayMessageRuntime.dll',
    'Plugins\GameplayMessageRouter\Binaries\Win64\UnrealEditor.modules'
)

foreach ($relativePath in $requiredFiles) {
    Assert-RequiredFile -Root $projectRoot -RelativePath $relativePath
}

$modulePath = Join-Path $projectRoot 'Binaries\Win64\UnrealEditor.modules'
$moduleJson = Get-Content -LiteralPath $modulePath -Raw | ConvertFrom-Json
$engineVersion = Get-EngineVersion -Root $resolvedEngineRoot
if ($engineVersion -and $moduleJson.BuildId -and $engineVersion.BuildId -and $moduleJson.BuildId -ne $engineVersion.BuildId) {
    Write-Warning "Prebuilt module BuildId ($($moduleJson.BuildId)) differs from engine BuildId ($($engineVersion.BuildId)). The editor may ask to rebuild."
}

$regPath = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
New-Item -Path $regPath -Force | Out-Null
New-ItemProperty -Path $regPath -Name $engineAssociation -Value $resolvedEngineRoot -PropertyType String -Force | Out-Null

Write-Host "EngineAssociation registered: $engineAssociation -> $resolvedEngineRoot"
Write-Host 'Prebuilt binaries are present.'
Write-Host 'You can now double-click ToyTowerDefense.uproject.'

if ($Open) {
    Start-Process -FilePath $editorExe -ArgumentList "`"$uprojectPath`""
}
