# Live Items Feature Pack

Status: `draft`, `feature-owned-native-pending`, `proved-build`

This pack describes the Live Items system: live database-backed item lookup, per-instance dynamic item data, admin item editing, and the native Item Forge input window.

## What This Feature Owns

- Live item ID range rules and cache polling.
- DB-backed item lookup fallback through `SharedDatabase`.
- Per-instance dynamic item modifiers stored on `ItemInstance`.
- Lua and Perl quest helpers for building and rewarding live item instances.
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

## Tester Harness

For public testbed setup, apply `features/live-items/sql/002_live_items_testbed_seed.sql`, then `#repop` or zone out/in. The standard public promotion command does this through `-ApplyFeatureSql`. This tester round seeds only the forge NPC, augment NPC, tester cache/template rows, and direct tutorialb mob loot. The Aelis epic quest path is not part of this test.

- `#itemforge dialog` opens the native Item Forge with all supported tester stat keys listed in the transport payload. The feature patcher installs this project's own `dinput8.dll` plus the Item Forge XML into the client.
- `#itemforge craft type=weapon name=Storm_Blade hp=50 mana=20 ac=5 damage=12 delay=24 haste=5 str=10 heroic_str=2` creates a DB-backed forged item for focused stat testing.
- `#itemforge random [count]` summons Live Items Test Loot Cache items; opening each cache creates a per-instance random item without allocating a new item row.
- Vedra Forgecall (`900901`) opens the forge and hands out cache batches.
- Orin Augspinner (`900902`) lets testers build a named shardwork augment, purchase stat upgrades with Blood Shards alternate currency, and receive a per-instance custom augment.
- Nalyx Augmentweaver (`900905`) lets testers say `catalyst` to receive an Augment Catalyst, then fuse one catalyst plus one to three augment items into a single dynamic augment.
- `features/live-items/quests/tutorialb/zone.lua` rolls one per-instance live item on each normal NPC spawn in `tutorialb` and attaches it directly to that NPC's loot list, so the corpse looks like it naturally had the item.

## Deferred Epic Quest Test Content

The Aelis class epic collection quest remains useful for later curated quest testing, but it is not seeded or included in the current public testbed checklist. Keep this tester round focused on live item generation, direct mob loot, Item Forge input, and augment generation.

## Quest API

Curated quests can create one-off live item instances without allocating new item IDs:

- Lua: `eq.create_live_item({ item_id = 900001, data = {...}, modifiers = {...}, custom_data = {...} })`, then `client:RewardLiveItem(inst)`.
- Lua NPC/corpse loot: `npc:AddLiveItem(inst, false)` or `corpse:AddLiveItem(inst)` to place a dynamic instance directly in loot.
- Perl: `quest::createliveitem({ item_id => 900001, data => {...}, modifiers => {...}, custom_data => {...} })`, then `$client->RewardLiveItem($inst)`.
- Perl NPC/corpse loot: `$npc->AddLiveItem($inst, 0)` or `$corpse->AddLiveItem($inst)` to place a dynamic instance directly in loot.

Perl also has `$client->PushItemOnCursor($inst)` and `$client->PutItemInInventory($slot_id, $inst)` for direct placement of curated instances.

## Native Client Caveat

This branch includes the native Item Forge XML. Any feature-specific native client behavior, transport parsing, slash-command rewriting, native EQ windows, or `dinput8.dll` work belongs in this feature checkout and should deploy only to the matching Live Items client until a proper native-client-base split exists.

## Client Patcher Sync

Client patch syncing is owned by `features/live-items/patcher.yml`. Do not add files directly to the local EQ client as the source of truth. Every external/test-client file must be committed in this repo and listed there as a `source` path plus the `destination` path inside the EverQuest client root.

The patcher can also generate project-local `eqhost.txt`, `EQUI.xml`, and EQUI include entries through the `generated` block. Use `generated.eqhost = true` for `eqhost.txt`, `generated.equiXml = true` for native UI XML injection, and list custom `EQUI_*.xml` windows explicitly in `generated.equiIncludes`. Missing listed files are release blockers for real external syncs; use `-AllowMissingClientFiles` only for partial local testing.

Regenerate and test the project feed from the patcher service:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
```

Resolve `<project-id>` from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`; it is the workspace install `id`, which usually matches the feature id but should not be assumed blindly. The feed publishes at `http://<patch-host>:8091/patcher/<project-id>/`. External testers place that project's generated `eqemupatcher.exe` in their EQ client root and run it.
