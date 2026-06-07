# AI NPC response Manifest

Feature id: `ai-npc-response`

## Prototype Files

- `features/ai-npc-response/ai-bridge/app/main.py`
- `features/ai-npc-response/ai-bridge/app/__init__.py`
- `features/ai-npc-response/ai-bridge/config/npcs.json`
- `features/ai-npc-response/ai-bridge/config/lore.json`
- `features/ai-npc-response/ai-bridge/config/online_lore.json`
- `features/ai-npc-response/ai-bridge/requirements.txt`
- `features/ai-npc-response/ai-bridge/scripts/*.ps1`
- `features/ai-npc-response/ai-bridge/samples/eqemu_ai_npc_sample.pl`
- `features/ai-npc-response/quests/tutorialb/900903.pl`
- `features/ai-npc-response/sql/001_sage_aurelian_tutorialb.sql`
- `features/ai-npc-response/patcher.yml`

## Runtime Targets

- Testbed bridge: `D:\EQEmu\Testbed\ai-bridge`
- Testbed quest: `D:\EQEmu\Testbed\server\quests\tutorialb\900903.pl`
- Testbed DB seed: `npc_types`, `spawngroup`, `spawnentry`, and `spawn2`
  rows for NPC/spawn id `900903`.

## Client Files

None for the current prototype. `patcher.yml` is intentionally empty until
native client UI or other client-facing files become part of the feature.

## Next Implementation Areas

- Replace the ad hoc online lore lookup with curated retrieval files and/or a
  small indexed lore corpus.
- Add protected quest keyword handling before generative fallback.
- Add per-NPC/player memory scoped by zone.
- Add faction, deity, class, race, and quest-state context inputs.
- Package a testbed deployment script once the VM path conventions stabilize.
