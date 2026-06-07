param(
  [string]$Url = "http://127.0.0.1:11434"
)

$ErrorActionPreference = "Stop"
$response = Invoke-RestMethod -Uri "$Url/api/tags" -TimeoutSec 5
$response
