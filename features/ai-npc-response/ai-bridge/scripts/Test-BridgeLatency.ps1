param(
  [int]$Count = 5,
  [string]$Url = "http://127.0.0.1:18080/eqemu/npc-chat"
)

$ErrorActionPreference = "Stop"
$samples = @()

for ($i = 1; $i -le $Count; $i++) {
  $payload = @{
    npc_id = 189119
    npc_name = "Arias"
    zone_short_name = "tutorialb"
    player_name = "Tester"
    player_message = "Hail, what should I do next?"
    recent_context = @("Tester hailed Arias.")
  } | ConvertTo-Json -Depth 5

  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  $result = Invoke-RestMethod -Method Post -Uri $Url -ContentType "application/json" -Body $payload -TimeoutSec 15
  $sw.Stop()
  $samples += [pscustomobject]@{
    attempt = $i
    wall_ms = [int]$sw.ElapsedMilliseconds
    bridge_ms = $result.latency_ms
    ok = $result.ok
    fallback = $result.fallback
    response = $result.response
  }
}

$avgWall = [int](($samples | Measure-Object -Property wall_ms -Average).Average)
$avgBridge = [int](($samples | Measure-Object -Property bridge_ms -Average).Average)

[pscustomobject]@{
  count = $Count
  average_wall_ms = $avgWall
  average_bridge_ms = $avgBridge
  samples = $samples
}
