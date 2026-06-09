param(
    [string]$RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$ServerPath = "D:\EQServers\EQServer-All-Features",
    [string]$BuildPreset = "win-msvc",
    [string]$BuildConfig = "Release",
    [string]$ExportTool = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [string[]]$Arguments = @(),
        [string]$WorkingDirectory = ""
    )

    $previousLocation = Get-Location
    if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) {
        Set-Location -LiteralPath $WorkingDirectory
    }

    try {
        & $FilePath @Arguments
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($Arguments -join ' ')"
        }
    }
    finally {
        Set-Location -LiteralPath $previousLocation
    }
}

$repoRootPath = (Resolve-Path -LiteralPath $RepoRoot).Path
$serverPathValue = (Resolve-Path -LiteralPath $ServerPath).Path
$serverConfigPath = Join-Path $serverPathValue "eqemu_config.json"

if (-not (Test-Path -LiteralPath $serverConfigPath)) {
    throw "ServerPath must contain eqemu_config.json: $serverPathValue"
}

if ([string]::IsNullOrWhiteSpace($ExportTool)) {
    $ExportTool = Join-Path $repoRootPath "build\$BuildPreset\bin\$BuildConfig\export_client_files.exe"
}

if (-not $SkipBuild) {
    $buildDir = Join-Path $repoRootPath "build\$BuildPreset"
    Invoke-Checked -FilePath "cmake" -Arguments @("--build", $buildDir, "--config", $BuildConfig, "--target", "export_client_files", "--", "/m") -WorkingDirectory $repoRootPath
}

$exportToolPath = (Resolve-Path -LiteralPath $ExportTool).Path
$serverExportDir = Join-Path $serverPathValue "export"
New-Item -ItemType Directory -Force -Path $serverExportDir | Out-Null

Invoke-Checked -FilePath $exportToolPath -WorkingDirectory $serverPathValue

$repoExportDir = Join-Path $repoRootPath "client_files\generated\all-features"
New-Item -ItemType Directory -Force -Path $repoExportDir | Out-Null

$requiredFiles = @(
    "spells_us.txt",
    "dbstr_us.txt",
    "SkillCaps.txt"
)

foreach ($fileName in $requiredFiles) {
    $sourcePath = Join-Path $serverExportDir $fileName
    $destinationPath = Join-Path $repoExportDir $fileName

    if (-not (Test-Path -LiteralPath $sourcePath)) {
        throw "Expected export file was not created: $sourcePath"
    }

    $sourceItem = Get-Item -LiteralPath $sourcePath
    if ($sourceItem.Length -le 0) {
        throw "Expected export file is empty: $sourcePath"
    }

    Copy-Item -LiteralPath $sourcePath -Destination $destinationPath -Force
    Write-Host "Updated $destinationPath ($($sourceItem.Length) bytes)"
}

Write-Host "All-features client data is ready for patcher publication."
