# Live Spells Manifest

This manifest lists the files and hook points that make up the standalone Live Spells feature.

## Added Server Files

- `zone/gm_commands/livespell.cpp`
- `zone/live_spell_manager.h`
- `docs/live-spells.md`
- `features/live-spells/patcher.yml`

## Existing Server Files To Patch

| File | Purpose |
| --- | --- |
| `common/shareddb.cpp` | Add live item lookup, cache, and DB fallback for generated scroll items. |
| `common/shareddb.h` | Declare live item cache APIs. |
| `common/ruletypes.h` | Add Live Item range and polling rules used by generated scroll items. |
| `world/world_boot.cpp` | Preserve shared item count before live fallback is enabled. |
| `zone/CMakeLists.txt` | Build `livespell.cpp` and list `live_spell_manager.h`. |
| `zone/command.cpp` | Register `#livespell`. |
| `zone/command.h` | Declare the Live Spells command handler. |
| `zone/spells.cpp` | Ensure persisted live spell definitions are loaded before casting. |
| `zone/client_process.cpp` | Ensure persisted live spell definitions are loaded before scribing/memorizing. |

## Database Objects

No custom tables are required. Live Spells use existing `items`, `rule_values`, and `data_buckets` tables.

Optional rules SQL lives in:

- `features/live-spells/sql/001_live_spell_runtime_rules.sql`

## Native Client Assets

| File | Purpose |
| --- | --- |
| `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` | Feature-owned client DLL published to the EQ client root by `patcher.yml`. |
| `client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml` | Native SIDL window layout for Spell Forge input. |

`features/live-spells/patcher.yml` maps client-facing files from this repo into the target EverQuest client folder. It also enables generated `eqhost`, generated `EQUI.xml`, and includes `EQUI_NativeSpellForgeWnd.xml`.

Do not add files directly to the local EQ client as the source of truth. The feature repo owns the external client payload; the EQ client folder is only a deployment target.

In `patcher.yml`, `files[].source` is a path inside this repo and `files[].destination` is a path inside the EverQuest client root. Use `generated.eqhost: true` when the patcher should write `eqhost.txt`, `generated.equiXml: true` when native UI XML includes need to be injected, and `generated.equiIncludes` to list custom `EQUI_*.xml` windows explicitly.

Missing files listed in `patcher.yml` are release blockers for real external syncs. Use `-AllowMissingClientFiles` only for partial local testing.

Before regenerating the external patch feed, look up the workspace install id in `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but do not assume that blindly. Then run from the patcher host:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
```

The feed is published at `http://<patch-host>:8091/patcher/<project-id>/`.

## Commands

- `#livespell dialog`
- `#livespell craft [fire|cold|magic|poison|disease] [target|ae|pbae] [range] [damage] [recast_ms]`
- `#livespell craft element=cold target=ae range=200 damage=100 recast=3000 name=Frost_Burst`
- `#livespell ready`
- `#livespell ack [spell_id] [version]`
- `#livespell patch [spell_id] [base_spell_id]`
- `#livespell test [spell_id] [base_spell_id] [gem]`
- `#livespell scribe [spell_id] [gem]`

## Native Transport

- `LIVESPELL|ui|open|...`
- `LIVESPELL|upsert|...`

The server owns spell generation, scroll creation, and validation. The native window is only an input surface.

If Live Spells needs native client behavior, transport parsing, slash-command rewriting, or native EQ windows, that client DLL code belongs in this checkout. Do not source this feature's `dinput8.dll` from `EQEmu-native-client-runtime`.
