# Advanced Loot Handoff

This project now treats loot as a live-style Advanced Loot system, not the old script-shaped AutoLoot feature.

## Current Direction

- Solo NPC kills populate the player Personal Loot list.
- Group NPC kills populate Shared Loot for the calculated Master Looter.
- Shared assignments move an item to the target player's Personal Loot list; the target still loots from there.
- Need/Greed rolls use a 60 second timeout, with Need beating Greed.
- Saved filters are fresh live-style values: `unset`, `always_need`, `always_greed`, `never`, plus a separate Auto Roll flag.
- Saved personal filters apply immediately: Always Need/Greed auto-loot personal rows, and Never leaves matching rows on the corpse.
- Auto Show is opt-in; the native window stays closed until `#advloot window` or an explicit native show request opens it.
- Old include/exclude filters are not migrated.
- Removed player-facing conveniences: Loot Nearby, AutoSell, round-robin/killer/assigned automation modes, `#lootfilter`, `#autosell`, and standalone `#needgreed`.

## Commands

Player-facing entry point:

```text
#advloot [status|window|on|off]
#advloot applyfilters [on|off]
#advloot autosplit [on|off]
#advloot autolootall [on|off]
#advloot masterlooter [on|off]
#advloot debug [on|off]
#advloot log [on|off]
#advloot inspect [Entry ID]
#advloot action [Entry ID] [loot|leave|never|need|greed|no|alwaysneed|alwaysgreed|ask|roll|freegrab|give]
#advloot filter [list|set|autoroll|remove]
```

Native transport is `ADVLOOT|...`.

## Data

Current tables:

```text
custom_advloot_settings
custom_advloot_filters
custom_advloot_audit
```

The version 19 custom migration drops the old `custom_autoloot_*` tables because this is a testbed and old data is intentionally discarded. Version 21 makes Auto Show opt-in and resets existing `custom_advloot_settings.auto_show_loot_window` values to `0`.

## Files

- Server engine: `zone/autoloot_manager.cpp`, `zone/autoloot_manager.h`
- Command registration: `zone/command.cpp`, `zone/gm_commands/autoloot.cpp`
- Native DLL runtime: `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h`
- Native XML: `client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml`
- Custom DB migration: `common/database/database_update_manifest_custom.h`
- SQL mirror: `features/all-features/sql/013_live_advanced_loot_schema.sql`
- Auto Show opt-in mirror: `features/all-features/sql/014_advloot_autoshow_opt_in.sql`

## Verification Focus

- Solo: kill mobs, verify Personal Loot rows, Loot, Leave, Never, Loot All, Apply Filters on/off.
- Group: verify Master Looter selection, shared rows, Ask/Roll, Auto Roll, 60 second timeout, Need over Greed, No/pass cases, direct Give to Personal Loot.
- Messaging: assigned or looted group items should still produce normal group-visible loot messages.
- UI: right-click inspect items, resize window, reload UI, logout/re-enter, Group by NPC, filter/settings windows.
- Safety: no duplication, inventory full behavior, corpse cleanup, locked/out-of-range rows.

## Build And Deploy

Use the all-features project commands:

```powershell
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32 /m
```

Then regenerate/deploy with the all-features patcher/testbed flow from `AGENTS.md`.
