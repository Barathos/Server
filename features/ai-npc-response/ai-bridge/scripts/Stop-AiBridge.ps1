$ErrorActionPreference = "Stop"
$BridgeRoot = Split-Path -Parent $PSScriptRoot
$PidFile = Join-Path $BridgeRoot "runtime\bridge.pid"

if (!(Test-Path -LiteralPath $PidFile)) {
  Write-Host "No bridge pid file found."
  return
}

$BridgePid = Get-Content -LiteralPath $PidFile | Select-Object -First 1
if ($BridgePid) {
  Stop-Process -Id ([int]$BridgePid) -ErrorAction SilentlyContinue
  Remove-Item -LiteralPath $PidFile -Force
  Write-Host "Stopped bridge PID $BridgePid"
}

Get-CimInstance Win32_Process |
  Where-Object {
    $_.CommandLine -like "*D:\EQEmu\Testbed\ai-bridge*" -and
    $_.CommandLine -like "*uvicorn*"
  } |
  ForEach-Object {
    Stop-Process -Id $_.ProcessId -ErrorAction SilentlyContinue
  }
