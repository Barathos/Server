# AI NPC response

Design and operator notes for the standalone `ai-npc-response` feature.

## Architecture

```text
EQEmu Perl quest -> localhost FastAPI bridge -> Ollama chat/generate API -> NPC speech
```

The bridge listens only on `127.0.0.1:18080` and calls Ollama on
`127.0.0.1:11434`. The public testbed should not expose either port.

## Current Prototype

Sage Aurelian is a Tutorial B test NPC (`npc_types.id = 900903`) placed near
Orin Augspinner and Vedra Forgecall. His quest script starts an async bridge
job, says a short thinking line for deeper lore requests, and polls for the
answer on a timer.

The bridge supports:

- `POST /eqemu/npc-chat` for phase 1 synchronous requests.
- `POST /eqemu/npc-chat/start` for async/polling requests.
- `GET /eqemu/npc-chat/result/{job_id}` for completed async results.
- `GET /health` for config/model/bind checks.

## Lore Guarding

The current guardrails are deliberately light so the pipeline can be tested:

- Compact NPC persona config in `ai-bridge/config/npcs.json`.
- Zone/global lore and exact-term fallbacks in `ai-bridge/config/lore.json`.
- Optional whitelisted MediaWiki sources in `ai-bridge/config/online_lore.json`.
- Output scrubbers for prompt leaks, OOC terms, and wiki infobox metadata.

For known terms like Quarm, the bridge prefers local lore/fallbacks over
generated guesses. For online snippets that expose a clean fact sentence, it
returns a deterministic in-character summary instead of asking the tiny model
to improvise.

## Testbed Install

From a checkout on the testbed VM, copy the bridge folder to:

```text
D:\EQEmu\Testbed\ai-bridge
```

Install/start:

```powershell
cd D:\EQEmu\Testbed\ai-bridge
.\scripts\Setup-AiBridge.ps1
.\scripts\Start-OllamaLocal.ps1 -Background
.\scripts\Test-Ollama.ps1
.\scripts\Start-AiBridge.ps1 -Background
Invoke-RestMethod http://127.0.0.1:18080/health
```

Apply `features/ai-npc-response/sql/001_sage_aurelian_tutorialb.sql` to the
testbed DB, then copy `features/ai-npc-response/quests/tutorialb/900903.pl` to
`D:\EQEmu\Testbed\server\quests\tutorialb\900903.pl`.

Reload the quest and repop Tutorial B in-game. No EQEmu service restart is
required for this prototype NPC/quest change.

## Latency Test

Use:

```powershell
cd D:\EQEmu\Testbed\ai-bridge
.\scripts\Test-BridgeLatency.ps1 -Count 5
```

Also test the async in-game path by asking Sage Aurelian:

- `hail`
- `who is quarm`
- `who is firiona vie`
- `tell me about karnor's castle`
- `where is my computer`

## Next Work

- Move from online wiki lookup to curated reference files or a local lore
  retrieval index.
- Add per-zone reference folders and per-NPC reference attachments.
- Protect real quest keywords before sending text to the model.
- Add per-player conversation memory with short TTLs.
- Compare `qwen2.5:0.5b`, `qwen2.5:3b`, and a 7B/8B model for response quality
  versus latency.
