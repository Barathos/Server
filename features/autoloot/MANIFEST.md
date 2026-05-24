# AutoLoot Manifest

This manifest lists the files and hook points that make up the source-backed native AutoLoot feature.

## Added Server Files

- `zone/autoloot_manager.cpp`
- `zone/autoloot_manager.h`
- `zone/gm_commands/autoloot.cpp`

## Existing Server Files To Patch

| File | Purpose |
| --- | --- |
| `zone/CMakeLists.txt` | Add AutoLoot manager and command source files to the zone target. |
| `zone/attack.cpp` | Notify AutoLoot when a corpse is created after a kill. |
| `zone/zone.cpp` | Tick the AutoLoot manager from the zone process loop. |
| `zone/command.cpp` | Register AutoLoot, AutoSell, LootFilter, and NeedGreed commands. |
| `zone/command.h` | Declare AutoLoot command handlers. |
| `zone/corpse.cpp` | Provide source-side corpse item transfer support for AutoLoot. |
| `zone/corpse.h` | Declare AutoLoot corpse result types and transfer API. |
| `common/database/database_update_manifest_custom.h` | Add the custom AutoLoot database migration. |
| `common/version.h` | Bump `CUSTOM_BINARY_DATABASE_VERSION` when adding the migration. |

## Runtime Hook Points

| Hook | Expected Behavior |
| --- | --- |
| Corpse creation in `zone/attack.cpp` | Queue nearby AutoLoot processing after an NPC dies. |
| Zone process loop in `zone/zone.cpp` | Drain AutoLoot work without relying on quest timers. |
| `Corpse::AutoLootItem` | Move items using server-side checks and corpse removal semantics. |
| Command registration | Expose player and admin control through normal server commands. |
| Database migration | Create persistent AutoLoot settings, filters, group settings, autosell exclusions, and optional audit rows. |

## Database Objects

Standalone SQL lives in:

- `features/autoloot/sql/001_source_backed_autoloot.sql`

Owned tables:

- `custom_autoloot_settings`
- `custom_autoloot_filters`
- `custom_autoloot_autosell_exclusions`
- `custom_autoloot_group_settings`
- `custom_autoloot_audit`

## Native Client Assets

| File | Purpose |
| --- | --- |
| `client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml` | Native SIDL window layout for AutoLoot. |
| `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` | Not included in this proof branch. The current implementation is still shared runtime code and needs a `native-client-base` split before this is a minimal client pack. |
| Target client `uifiles/default/EQUI.xml` | Must include `EQUI_NativeAutoLootWnd.xml` so the client loads the window. |

The client source currently needs further separation before this can be called a minimal client pack. This branch proves the server-side AutoLoot feature boundary first.

## Player Commands

- `#autoloot native show`
- `#autoloot native status`
- `#lootfilter native list both`
- `/lootfilter keep <item_id>`
- `/lootfilter ignore <item_id>`
- `/lootfilter unset <item_id>`
- `/autosell`
- `/needgreed vote <id> <need|greed|pass>`

## Native Transport

Server-to-client lines use the hidden chat protocol prefix:

- `AUTOLOOT|...`

Client-to-server actions are sent as ordinary server commands through the DLL command bridge. The client must not be trusted to decide whether an item can be moved, sold, rolled, or awarded.

## Known Non-Goals

- Do not restore Perl, MQ, Lua, or overlay AutoLoot.
- Do not summon copied items to players as a loot shortcut.
- Do not trust the native client window as authoritative state.
- Do not bundle Live Items, Live Spells, or Achievements into the AutoLoot server pack.
