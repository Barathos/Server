[CmdletBinding()]
param(
    [string]$BaseUrl = "http://localhost:8080/patcher/",
    [string]$Client = "rof",
    [string]$PatcherFileName = "eqemupatcher"
)

$ErrorActionPreference = "Stop"

if (-not $BaseUrl.EndsWith("/")) {
    $BaseUrl += "/"
}

$endpoints = @(
    "",
    "patch_notes.txt",
    "patcher_status.yml",
    "$Client/filelist_$Client.yml"
)

foreach ($endpoint in $endpoints) {
    $url = "$BaseUrl$endpoint"
    $response = Invoke-WebRequest -Uri $url -UseBasicParsing
    if ($response.StatusCode -lt 200 -or $response.StatusCode -ge 300) {
        throw "Unexpected status $($response.StatusCode) for $url"
    }
    Write-Host "OK $url"
}

$hashUrl = "$BaseUrl$PatcherFileName-hash.txt"
try {
    $response = Invoke-WebRequest -Uri $hashUrl -UseBasicParsing
    if ($response.Content.Trim().Length -gt 0) {
        Write-Host "OK $hashUrl"
    }
}
catch {
    Write-Warning "No patcher self-update hash was found at $hashUrl. This is expected until a patcher exe is published."
}
