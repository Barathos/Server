[CmdletBinding()]
param(
    [string]$TaskName = "EQEmuPatcherService",
    [string]$Root = "",
    [string]$UrlPrefix = "http://+:8080/patcher/"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Root)) {
    $Root = Join-Path $PSScriptRoot "publish"
}

$rootPath = [System.IO.Path]::GetFullPath($Root)
$scriptPath = Join-Path $PSScriptRoot "Start-PatcherService.ps1"
if (-not (Test-Path -LiteralPath $rootPath)) {
    throw "Publish root does not exist: $rootPath"
}

netsh http add urlacl url=$UrlPrefix user=Everyone | Out-Host
if (Get-Command New-NetFirewallRule -ErrorAction SilentlyContinue) {
    $port = ([uri]($UrlPrefix.Replace("+", "localhost"))).Port
    if (-not (Get-NetFirewallRule -DisplayName $TaskName -ErrorAction SilentlyContinue)) {
        New-NetFirewallRule -DisplayName $TaskName -Direction Inbound -Action Allow -Protocol TCP -LocalPort $port | Out-Null
    }
}

$taskCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$scriptPath`" -Root `"$rootPath`" -UrlPrefix `"$UrlPrefix`""
schtasks.exe /Create /F /SC ONSTART /TN $TaskName /TR $taskCommand /RU SYSTEM | Out-Host

Write-Host "Installed startup task $TaskName for $UrlPrefix" -ForegroundColor Green
Write-Host "Start now with: schtasks /Run /TN $TaskName"
