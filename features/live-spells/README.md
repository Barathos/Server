# Live Spells Feature Pack

Status: `draft`, `shared-runtime`, `proved-build`

This pack describes the Live Spells system: generated spell definitions, generated scroll items, server-side spell patching, and the native Spell Forge input window.

## What This Feature Owns

- Reserved spell ID range `42500-42602` for generated spells.
- Generated spell definitions persisted in `data_buckets`.
- Server-side spell array patching when a zone first needs live spell data.
- Generated scroll items stored in `items`.
- Live item lookup support needed by generated scroll item IDs.
- `#livespell` command paths for dialog, crafting, sync, GM patch/test, and scribing.
- Native Spell Forge UI transport using `LIVESPELL|...` lines.

## What This Feature Does Not Require

- AutoLoot.
- Item Forge or item editing commands.
- Achievements.
- MacroQuest, Lua, ImGui, or overlay UI code.

## Dependencies

- EQEmu source rebuild.
- Blank spell rows in the reserved generated spell range.
- `rule_values` rows if the operator wants to override the generated scroll item live range.
- The native client DLL host if the operator wants the in-client Spell Forge window.

## Install Outline

1. Apply the source files and hook patches listed in `MANIFEST.md`.
2. Optionally apply `sql/001_live_spell_runtime_rules.sql` to make generated scroll item live lookup explicit.
3. Rebuild `zone` and `world`.
4. Deploy `EQUI_NativeSpellForgeWnd.xml` and include it from the target client's `EQUI.xml`.
5. Use `#livespell dialog`, then craft a generated spell scroll.

## Smoke Test

1. Run `#livespell dialog` and confirm the native client receives the Spell Forge open line.
2. Run `#livespell craft element=fire target=target range=200 damage=100 recast=3000 name=Test_Flame`.
3. Confirm a generated scroll item is created in the live item range and placed on the cursor.
4. Scribe the scroll, memorize the generated spell, then cast it.
5. Restart a zone and confirm `#livespell ready` resyncs persisted generated spells.

## Current Shared-Runtime Caveat

This branch includes the native Spell Forge XML only. The C++ DLL implementation is still in the lab branch's shared native-client runtime header and should be split into `native-client-base` plus feature-specific native window code.
