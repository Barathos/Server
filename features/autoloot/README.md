# AutoLoot Feature Pack

Status: `draft`, `shared-runtime`

This pack describes the source-backed AutoLoot system with the native EverQuest AutoLoot window. It is intended for a server operator who wants AutoLoot without also taking Live Items, Live Spells, or Achievements.

## What This Feature Owns

- Character AutoLoot settings.
- Per-character item filters using player-facing `Keep`, `Ignore`, and `Unset` language.
- Group loot settings, shared decision queues, and Need/Greed voting.
- AutoSell preview and confirm flow.
- Nearby corpse processing.
- Server-side item transfer through source-controlled corpse loot paths.
- Native AutoLoot UI transport using `AUTOLOOT|...` chat protocol lines.

## What This Feature Does Not Require

- Live Items.
- Live Spells.
- Achievements.
- Old Perl `autoloot.pl` execution.
- MacroQuest, Lua, ImGui, or an overlay AutoLoot window.

The old script and MQ/Lua approaches should stay out of this pack. The native client window is now the AutoLoot UI surface; the server remains authoritative for all loot decisions and item movement.

## Dependencies

- EQEmu source rebuild.
- Custom database update support, or manual execution of the SQL in this pack.
- The native client DLL host used by this branch if the operator wants the in-client AutoLoot window.

The command surface can still be used without the native window, but the intended package is the native window plus server source.

## Current Shared-Runtime Caveat

The server-side AutoLoot boundary is reasonably separable now.

The client-side boundary is not fully clean yet: the lab branch's `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` currently contains AutoLoot code alongside other native custom windows such as Item Forge, Spell Forge, and Achievements. A truly minimal AutoLoot client pack should extract a small native-client base and then move AutoLoot-specific code into its own client source file.

Until that split is done, this proof branch includes the AutoLoot UI XML but not the shared DLL source. Operators should treat the client DLL portion as shared runtime code and copy only the AutoLoot-specific C++ portions with care.

## Install Outline

1. Apply the server source files and hook patches listed in `MANIFEST.md`.
2. Apply the database schema from `sql/001_source_backed_autoloot.sql`, or port it into the server's custom migration system.
3. Rebuild `zone`.
4. Deploy the native client DLL host and `EQUI_NativeAutoLootWnd.xml`.
5. Add `EQUI_NativeAutoLootWnd.xml` to the target client's `EQUI.xml` include list.
6. Log in and run `#autoloot native show`.

## Smoke Test

1. Kill an NPC with loot.
2. Open the native AutoLoot window with `#autoloot native show`.
3. Mark an item `Keep`, then kill another NPC that drops the same item.
4. Confirm the server moves the item through the source loot path and leaves blocked loot on the corpse.
5. Mark an item `Ignore` and confirm it is left alone.
6. In a group, verify Need/Greed/Pass decisions use the server queue and do not trust client-only state.

## Next Split Work

- Extract AutoLoot-specific client code out of `core_autoloot_native.h`.
- Create a `native-client-base` pack for shared DLL hooks, chat transport plumbing, and safe window lifecycle helpers.
- Generate an AutoLoot-only patch from the manifest once the client-side split is clean.
