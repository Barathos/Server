[CmdletBinding()]
param(
    [string]$WorkspaceRoot = "D:\Codex\Apps\EQEmu-feature-workspaces",
    [string[]]$Project = @(),
    [string]$Client = "rof",
    [string]$BaseUrl = "http://localhost:8091/patcher/",
    [string]$PatcherFileName = "eqemupatcher"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$workspaceScripts = Join-Path $WorkspaceRoot "scripts\InstallWorkspace.ps1"
if (-not (Test-Path -LiteralPath $workspaceScripts)) {
    throw "Workspace scripts were not found: $workspaceScripts"
}
. $workspaceScripts

if (-not $BaseUrl.EndsWith("/")) {
    $BaseUrl += "/"
}

$installManifest = Get-InstallManifest -ManifestPath (Join-Path $WorkspaceRoot "installs.json")
$installs = if ($Project.Count -gt 0) {
    foreach ($id in $Project) {
        Get-InstallById -Manifest $installManifest -Id $id
    }
}
else {
    @($installManifest.installs)
}

$tester = Join-Path $PSScriptRoot "Test-PatcherService.ps1"
if (-not (Test-Path -LiteralPath $tester)) {
    throw "Patcher service tester was not found: $tester"
}

$root = Invoke-WebRequest -Uri $BaseUrl -UseBasicParsing
Write-Host "OK $BaseUrl ($($root.StatusCode))"
$workspaceYaml = Invoke-WebRequest -Uri "$($BaseUrl)workspace_patchers.yml" -UseBasicParsing
Write-Host "OK $($BaseUrl)workspace_patchers.yml ($($workspaceYaml.StatusCode))"

$feedManifest = $null
try {
    $manifestResponse = Invoke-WebRequest -Uri "$($BaseUrl)workspace_patchers.json" -UseBasicParsing
    $feedManifest = $manifestResponse.Content | ConvertFrom-Json
}
catch {
    Write-Host "Workspace manifest was not available; falling back to client suffix '$Client'." -ForegroundColor Yellow
}

foreach ($install in $installs) {
    $feedBaseUrl = "$BaseUrl$($install.id)/"
    $feedClient = $Client
    if ($null -ne $feedManifest) {
        $feed = @($feedManifest.feeds | Where-Object { $_.id -eq $install.id } | Select-Object -First 1)
        if ($feed.Count -gt 0 -and -not [string]::IsNullOrWhiteSpace($feed[0].patchClient)) {
            $feedClient = $feed[0].patchClient
        }
    }
    Write-Host ""
    Write-Host "== Testing $($install.id) =="
    & $tester -BaseUrl $feedBaseUrl -Client $feedClient -PatcherFileName $PatcherFileName
}
