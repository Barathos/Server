param(
  [string]$PythonPath = ""
)

$ErrorActionPreference = "Stop"
$BridgeRoot = Split-Path -Parent $PSScriptRoot
$VenvPython = Join-Path $BridgeRoot ".venv\Scripts\python.exe"

function Find-Python {
  if ($PythonPath -and (Test-Path -LiteralPath $PythonPath)) {
    return $PythonPath
  }

  $candidates = @(
    "$env:LOCALAPPDATA\Programs\Python\Python312\python.exe",
    "$env:LOCALAPPDATA\Programs\Python\Python313\python.exe",
    "C:\Program Files\Python312\python.exe",
    "C:\Program Files\Python313\python.exe"
  )

  foreach ($candidate in $candidates) {
    if (Test-Path -LiteralPath $candidate) {
      return $candidate
    }
  }

  $cmd = Get-Command python -ErrorAction SilentlyContinue
  if ($cmd -and $cmd.Source -notmatch "\\WindowsApps\\python\.exe$") {
    return $cmd.Source
  }

  throw "Python 3.12+ was not found. Install Python first, then rerun this script."
}

$Python = Find-Python
Write-Host "Using Python: $Python"

if (!(Test-Path -LiteralPath $VenvPython)) {
  & $Python -m venv (Join-Path $BridgeRoot ".venv")
}

& $VenvPython -m pip install --upgrade pip
& $VenvPython -m pip install -r (Join-Path $BridgeRoot "requirements.txt")

Write-Host "Bridge Python environment is ready: $VenvPython"
