[CmdletBinding()]
param(
    [string]$TaskName = "EQEmuPatcherService",
    [string]$UrlPrefix = "http://+:8080/patcher/"
)

$ErrorActionPreference = "Stop"

schtasks.exe /Delete /F /TN $TaskName | Out-Host
netsh http delete urlacl url=$UrlPrefix | Out-Host

if (Get-Command Get-NetFirewallRule -ErrorAction SilentlyContinue) {
    Get-NetFirewallRule -DisplayName $TaskName -ErrorAction SilentlyContinue | Remove-NetFirewallRule
}

Write-Host "Removed patcher service task and URL reservation." -ForegroundColor Green
