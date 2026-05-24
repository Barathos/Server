# Live Spells Manifest

This manifest lists the files and hook points that make up the standalone Live Spells feature.

## Added Server Files

- `zone/gm_commands/livespell.cpp`
- `zone/live_spell_manager.h`
- `docs/live-spells.md`

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
| `client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml` | Native SIDL window layout for Spell Forge input. |
| `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` | Not included in this proof branch. The current implementation is still shared runtime code and needs a `native-client-base` split. |

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
