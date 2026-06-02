# AI NPC response

Standalone source for the `ai-npc-response` EQEmu prototype.

The current prototype connects an EQEmu Perl quest NPC to a local-only FastAPI
bridge, which calls Ollama and returns short in-character speech.

## Contents

- `ai-bridge/`: FastAPI bridge, config, startup scripts, and latency tests.
- `quests/tutorialb/900903.pl`: async/polling quest script for Sage Aurelian.
- `sql/001_sage_aurelian_tutorialb.sql`: idempotent seed for the Tutorial B
  test NPC and spawn.
- `patcher.yml`: currently empty because this phase has no client-facing files.

## Testbed Target

- Bridge install: `D:\EQEmu\Testbed\ai-bridge`
- EQEmu server: `D:\EQEmu\Testbed\server`
- Quest target: `D:\EQEmu\Testbed\server\quests\tutorialb\900903.pl`
- Bridge endpoint: `http://127.0.0.1:18080`
- Ollama endpoint: `http://127.0.0.1:11434`

Keep both Ollama and the bridge bound to localhost. Do not expose the bridge
publicly.

## Prototype NPC

- NPC type id: `900903`
- Name: `Sage_Aurelian`
- Zone: `tutorialb`
- Location: near Orin Augspinner and Vedra Forgecall

The quest script uses `POST /eqemu/npc-chat/start` and polls
`GET /eqemu/npc-chat/result/{job_id}` so deeper lore lookups do not hold the
zone process for the full model/search latency.

## Model Notes

`scripts/Start-AiBridge.ps1` currently defaults the testbed to
`qwen2.5:0.5b` for very fast iteration. The bridge code default is
`qwen2.5:3b` if launched without that script, and the model can be overridden:

```powershell
.\scripts\Start-AiBridge.ps1 -Model qwen2.5:3b
```

The richer model is better for flavor; the smaller model is useful while
testing guardrails and latency.

## Validation

From the bridge install directory on the VM:

```powershell
.\scripts\Setup-AiBridge.ps1
.\scripts\Start-OllamaLocal.ps1 -Background
.\scripts\Test-Ollama.ps1
.\scripts\Start-AiBridge.ps1 -Background
Invoke-RestMethod http://127.0.0.1:18080/health
.\scripts\Test-BridgeLatency.ps1 -Count 5
```

For the EQEmu side, apply the SQL seed, copy the quest script into the
Tutorial B quest folder, then use in-game quest reload/repop tools. This does
not require an EQEmu service restart.
