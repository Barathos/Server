param(
  [switch]$Background,
  [string]$HostAddress = "127.0.0.1:11434"
)

$ErrorActionPreference = "Stop"
$BridgeRoot = Split-Path -Parent $PSScriptRoot
$RuntimeDir = Join-Path $BridgeRoot "runtime"
$ModelsDir = Join-Path $BridgeRoot "ollama-models"
$Ollama = (Get-Command ollama -ErrorAction Stop).Source

New-Item -ItemType Directory -Force -Path $RuntimeDir | Out-Null
New-Item -ItemType Directory -Force -Path $ModelsDir | Out-Null

$env:OLLAMA_HOST = $HostAddress
$env:OLLAMA_MODELS = $ModelsDir
$env:OLLAMA_NO_CLOUD = "true"

if ($Background) {
  $Log = Join-Path $RuntimeDir "ollama.wmi.log"
  $escapedOllama = $Ollama.Replace("'", "''")
  $command = "cmd.exe /c set `"OLLAMA_HOST=$HostAddress`" && set `"OLLAMA_MODELS=$ModelsDir`" && set `"OLLAMA_NO_CLOUD=true`" && `"$escapedOllama`" serve >> `"$Log`" 2>>&1"
  $proc = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
    CommandLine = $command
    CurrentDirectory = $BridgeRoot
  }
  if ($proc.ReturnValue -ne 0) {
    throw "Failed to start Ollama via WMI. ReturnValue=$($proc.ReturnValue)"
  }
  $proc.ProcessId | Set-Content -LiteralPath (Join-Path $RuntimeDir "ollama.pid") -Encoding ascii
  Write-Host "Started Ollama command PID $($proc.ProcessId) on $HostAddress"
  Write-Host "Models: $ModelsDir"
  Write-Host "Log: $Log"
} else {
  & $Ollama serve
}
