$ErrorActionPreference = "Stop"
$BridgeRoot = Split-Path -Parent $PSScriptRoot
$PidFile = Join-Path $BridgeRoot "runtime\ollama.pid"

if (Test-Path -LiteralPath $PidFile) {
  $OllamaPid = Get-Content -LiteralPath $PidFile | Select-Object -First 1
  if ($OllamaPid) {
    Stop-Process -Id ([int]$OllamaPid) -ErrorAction SilentlyContinue
  }
  Remove-Item -LiteralPath $PidFile -Force
}

Get-Process -ErrorAction SilentlyContinue |
  Where-Object { $_.ProcessName -eq "ollama" -and $_.Path -like "*\Ollama\ollama.exe" } |
  Stop-Process -ErrorAction SilentlyContinue

Write-Host "Stopped local Ollama processes started for the testbed bridge."
