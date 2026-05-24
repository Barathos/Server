# Live Items Feature Pack

Status: `draft`, `shared-runtime`, `proved-build`

This pack describes the Live Items system: live database-backed item lookup, per-instance dynamic item data, admin item editing, and the native Item Forge input window.

## What This Feature Owns

- Live item ID range rules and cache polling.
- DB-backed item lookup fallback through `SharedDatabase`.
- Per-instance dynamic item modifiers stored on `ItemInstance`.
- Item serialization that sends dynamic item data to supported clients.
- Held item refresh hooks so live DB edits can update in player inventory.
- `#liveitem`, `#itemedit`, and `#itemforge` commands.
- Native Item Forge UI transport using `LIVEITEM|...` lines.

## What This Feature Does Not Require

- AutoLoot.
- Live Spells.
- Achievements.
- MacroQuest, Lua, ImGui, or overlay UI code.

## Dependencies

- EQEmu source rebuild.
- `rule_values` rows if the operator wants to override the default live item range.
- The native client DLL host if the operator wants the in-client Item Forge window.

## Install Outline

1. Apply the source files and hook patches listed in `MANIFEST.md`.
2. Optionally apply `sql/001_live_items_rules.sql` to make the live item range explicit in the database.
3. Rebuild `zone` and `world`.
4. Deploy `EQUI_NativeItemForgeWnd.xml` and include it from the target client's `EQUI.xml`.
5. Use `#liveitem status`, then try `#itemforge dialog`.

## Smoke Test

1. Run `#liveitem status 900001`.
2. Run `#liveitem clone 900001 <known_source_item_id>`.
3. Run `#liveitem summon 900001`.
4. Run `#liveitem bump 900001 25 25 2` and confirm the held item refreshes after the poll interval.
5. Open `#itemforge dialog`, craft an item, and confirm the generated item is in the configured live item range.

## Current Shared-Runtime Caveat

This branch includes the native Item Forge XML only. The C++ DLL implementation is still in the lab branch's shared native-client runtime header and should be split into `native-client-base` plus feature-specific native window code.
