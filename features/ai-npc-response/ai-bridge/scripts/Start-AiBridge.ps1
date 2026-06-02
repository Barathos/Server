param(
  [switch]$Background,
  [string]$Model = "",
  [int]$Port = 18080
)

$ErrorActionPreference = "Stop"
$BridgeRoot = Split-Path -Parent $PSScriptRoot
$Python = Join-Path $BridgeRoot ".venv\Scripts\python.exe"
$RuntimeDir = Join-Path $BridgeRoot "runtime"

if (!(Test-Path -LiteralPath $Python)) {
  throw "Bridge venv not found. Run .\scripts\Setup-AiBridge.ps1 first."
}

New-Item -ItemType Directory -Force -Path $RuntimeDir | Out-Null

$env:OLLAMA_URL = if ($env:OLLAMA_URL) { $env:OLLAMA_URL } else { "http://127.0.0.1:11434" }
$env:OLLAMA_MODEL = if ($Model) { $Model } elseif ($env:OLLAMA_MODEL) { $env:OLLAMA_MODEL } else { "qwen2.5:0.5b" }
$env:OLLAMA_DEEP_MODEL = if ($env:OLLAMA_DEEP_MODEL) { $env:OLLAMA_DEEP_MODEL } else { $env:OLLAMA_MODEL }
$env:OLLAMA_KEEP_ALIVE = if ($env:OLLAMA_KEEP_ALIVE) { $env:OLLAMA_KEEP_ALIVE } else { "10m" }
$env:OLLAMA_TIMEOUT_SECONDS = if ($env:OLLAMA_TIMEOUT_SECONDS) { $env:OLLAMA_TIMEOUT_SECONDS } else { "6.0" }
$env:OLLAMA_DEEP_TIMEOUT_SECONDS = if ($env:OLLAMA_DEEP_TIMEOUT_SECONDS) { $env:OLLAMA_DEEP_TIMEOUT_SECONDS } else { "8.0" }
$env:OLLAMA_PREWARM_TIMEOUT_SECONDS = if ($env:OLLAMA_PREWARM_TIMEOUT_SECONDS) { $env:OLLAMA_PREWARM_TIMEOUT_SECONDS } else { "45.0" }
$env:OLLAMA_NUM_PREDICT = if ($env:OLLAMA_NUM_PREDICT) { $env:OLLAMA_NUM_PREDICT } else { "36" }
$env:OLLAMA_DEEP_NUM_PREDICT = if ($env:OLLAMA_DEEP_NUM_PREDICT) { $env:OLLAMA_DEEP_NUM_PREDICT } else { "72" }
$env:OLLAMA_TEMPERATURE = if ($env:OLLAMA_TEMPERATURE) { $env:OLLAMA_TEMPERATURE } else { "0.25" }
$env:ONLINE_LORE_LOOKUP = if ($env:ONLINE_LORE_LOOKUP) { $env:ONLINE_LORE_LOOKUP } else { "1" }

$ArgsList = @(
  "-m", "uvicorn", "app.main:app",
  "--host", "127.0.0.1",
  "--port", "$Port",
  "--log-level", "info"
)

if ($Background) {
  $Log = Join-Path $RuntimeDir "bridge.wmi.log"
  $command = "cmd.exe /c set `"OLLAMA_URL=$env:OLLAMA_URL`" && set `"OLLAMA_MODEL=$env:OLLAMA_MODEL`" && set `"OLLAMA_DEEP_MODEL=$env:OLLAMA_DEEP_MODEL`" && set `"OLLAMA_KEEP_ALIVE=$env:OLLAMA_KEEP_ALIVE`" && set `"OLLAMA_TIMEOUT_SECONDS=$env:OLLAMA_TIMEOUT_SECONDS`" && set `"OLLAMA_DEEP_TIMEOUT_SECONDS=$env:OLLAMA_DEEP_TIMEOUT_SECONDS`" && set `"OLLAMA_PREWARM_TIMEOUT_SECONDS=$env:OLLAMA_PREWARM_TIMEOUT_SECONDS`" && set `"OLLAMA_NUM_PREDICT=$env:OLLAMA_NUM_PREDICT`" && set `"OLLAMA_DEEP_NUM_PREDICT=$env:OLLAMA_DEEP_NUM_PREDICT`" && set `"OLLAMA_TEMPERATURE=$env:OLLAMA_TEMPERATURE`" && set `"ONLINE_LORE_LOOKUP=$env:ONLINE_LORE_LOOKUP`" && `"$Python`" -m uvicorn app.main:app --host 127.0.0.1 --port $Port --log-level info >> `"$Log`" 2>>&1"
  $proc = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
    CommandLine = $command
    CurrentDirectory = $BridgeRoot
  }
  if ($proc.ReturnValue -ne 0) {
    throw "Failed to start bridge via WMI. ReturnValue=$($proc.ReturnValue)"
  }
  $proc.ProcessId | Set-Content -LiteralPath (Join-Path $RuntimeDir "bridge.pid") -Encoding ascii
  Write-Host "Started EQEmu AI bridge command PID $($proc.ProcessId) on 127.0.0.1:$Port using model $env:OLLAMA_MODEL"
  Write-Host "Log: $Log"
} else {
  Set-Location $BridgeRoot
  & $Python @ArgsList
}
