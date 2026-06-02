# EQEmu AI NPC Bridge Prototype

Local-only FastAPI bridge for EQEmu NPC dialogue on the public testbed VM.

## Paths

- Source root in this repo: `features\ai-npc-response\ai-bridge`
- Testbed bridge root: `D:\EQEmu\Testbed\ai-bridge`
- Testbed EQEmu server root: `D:\EQEmu\Testbed\server`
- Safe sample quest scripts: `D:\EQEmu\Testbed\ai-bridge\samples`

The bridge binds to `127.0.0.1:18080` only. Do not expose this port publicly.

## Model

The testbed startup script defaults to `qwen2.5:0.5b` for fast iteration:

```powershell
.\scripts\Start-AiBridge.ps1
```

Override the model when testing richer responses:

```powershell
.\scripts\Start-AiBridge.ps1 -Model qwen2.5:3b
```

For a 7B/8B follow-up, check RAM and response time first.

## Setup

Run from the bridge root on the VM:

```powershell
cd D:\EQEmu\Testbed\ai-bridge
.\scripts\Setup-AiBridge.ps1
```

Confirm Ollama is listening locally:

```powershell
.\scripts\Start-OllamaLocal.ps1 -Background
.\scripts\Test-Ollama.ps1
```

Pull a model if needed:

```powershell
ollama pull qwen2.5:0.5b
ollama pull qwen2.5:3b
```

Start the bridge in the foreground:

```powershell
.\scripts\Start-AiBridge.ps1
```

Or start it in the background:

```powershell
.\scripts\Start-AiBridge.ps1 -Background
```

## Endpoints

- `GET /health`
- `POST /eqemu/npc-chat`
- `POST /eqemu/npc-chat/start`
- `GET /eqemu/npc-chat/result/{job_id}`

`/eqemu/npc-chat/start` returns immediate hail/OOC/advice responses when it
can. Deeper lore requests return a job id and a short acknowledgment so the
quest script can poll instead of blocking the zone process.

## Test

Health check:

```powershell
Invoke-RestMethod http://127.0.0.1:18080/health
```

Single synchronous request:

```powershell
$payload = @{
  npc_id = 900903
  npc_name = 'Sage Aurelian'
  zone_short_name = 'tutorialb'
  player_name = 'Tester'
  player_message = 'who is quarm'
} | ConvertTo-Json

Invoke-RestMethod -Method Post -Uri http://127.0.0.1:18080/eqemu/npc-chat -ContentType 'application/json' -Body $payload
```

Latency sample:

```powershell
.\scripts\Test-BridgeLatency.ps1 -Count 5
```

## EQEmu Quest

The deployed Tutorial B prototype quest is tracked at:

```text
features\ai-npc-response\quests\tutorialb\900903.pl
```

On the testbed VM, it belongs at:

```text
D:\EQEmu\Testbed\server\quests\tutorialb\900903.pl
```

The test NPC/spawn seed is:

```text
features\ai-npc-response\sql\001_sage_aurelian_tutorialb.sql
```

Reload the quest and repop Tutorial B in-game after copying/applying the
tracked files. No EQEmu service restart is required for this prototype.

## Next Improvements

- Replace online wiki lookup with curated reference files or local retrieval.
- Add per-NPC memory scoped by player and zone.
- Add faction, race, class, deity, and quest-state prompt inputs.
- Add protected quest keyword handling before generative fallback.
- Run broader model comparisons once the guardrails are more stable.
