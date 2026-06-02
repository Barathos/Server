# Live Items Manifest

This manifest lists the files and hook points that make up the standalone Live Items feature.

## Added Server Files

- `zone/gm_commands/itemedit.cpp`
- `zone/gm_commands/liveitem.cpp`
- `docs/live-items.md`
- `features/live-items/quests/global/global_player.lua`
- `features/live-items/quests/global/900901.lua`
- `features/live-items/quests/global/900902.lua`
- `features/live-items/quests/global/900904.lua`
- `features/live-items/quests/global/900905.lua`
- `features/live-items/quests/global/900906.lua`
- `features/live-items/quests/tutorialb/900905.lua`
- `features/live-items/quests/tutorialb/900906.lua`
- `features/live-items/quests/global/items/199091.lua`
- `features/live-items/quests/tutorialb/zone.lua`
- `features/live-items/TESTER_CHECKLIST.md`
- `features/live-items/TESTER_GOOGLE_FORM.md`
- `features/live-items/GOOGLE_FORM_PIPELINE.md`
- `features/live-items/google-forms/live-items-public-test-form.gs`

## Existing Server Files To Patch

| File | Purpose |
| --- | --- |
| `common/item_instance.cpp` | Add dynamic item data storage and application. |
| `common/item_instance.h` | Declare dynamic item APIs. |
| `common/shareddb.cpp` | Add live item lookup, cache, and DB fallback. |
| `common/shareddb.h` | Declare live item cache APIs. |
| `common/ruletypes.h` | Add Live Item range and polling rules. |
| `world/world_boot.cpp` | Preserve shared item count before live fallback is enabled. |
| `zone/CMakeLists.txt` | Build `itemedit.cpp` and `liveitem.cpp`. |
| `zone/command.cpp` | Register `#liveitem`, `#itemedit`, and `#itemforge`. |
| `zone/command.h` | Declare Live Item command handlers. |
| `zone/client.cpp` | Refresh live items during relevant client flows. |
| `zone/client.h` | Declare live item refresh helpers. |
| `zone/inventory.cpp` | Refresh held/inventory live item trees. |
| `zone/embparser_api.cpp` | Expose `quest::createliveitem` for Perl quest scripts. |
| `zone/lua_client.cpp` | Expose live item reward helper to Lua clients. |
| `zone/lua_client.h` | Declare Lua live item reward helper. |
| `zone/lua_general.cpp` | Expose `eq.create_live_item` for Lua quest scripts. |
| `zone/lua_iteminst.cpp` | Expose dynamic item helpers to Lua. |
| `zone/lua_iteminst.h` | Declare Lua dynamic item bindings. |
| `zone/perl_client.cpp` | Expose client refresh and live item reward helpers to Perl. |
| `zone/perl_questitem.cpp` | Expose dynamic item helpers to Perl quest items. |
| `tests/CMakeLists.txt` | Add dynamic item tests. |
| `tests/dynamic_item_test.h` | Test dynamic item behavior. |
| `tests/main.cpp` | Include dynamic item tests. |

## Database Objects

No custom tables are required. Live items use existing `items`, `rule_values`, and `data_buckets` tables.

Optional rules SQL lives in:

- `features/live-items/sql/001_live_items_rules.sql`
- `features/live-items/sql/002_live_items_testbed_seed.sql`
- `features/live-items/sql/003_live_items_log_settings.sql`
- `features/live-items/sql/004_augment_fusion_testbed_seed.sql`

## Native Client Assets

| File | Purpose |
| --- | --- |
| `features/live-items/patcher.yml` | Client patcher feed manifest for Live Items external/test-client syncs. |
| `client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml` | Native SIDL window layout for Item Forge input. |
| `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` | Feature-owned native client implementation for the Live Items Item Forge window and transport. |
| `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` | Feature-owned native DLL installed only by the Live Items patcher feed. |

External/test-client files must be committed in this repo and listed in `features/live-items/patcher.yml`; do not use a local EQ client folder as source of truth. Missing listed files block real external syncs; use `-AllowMissingClientFiles` only for partial local testing. Resolve the patcher deployment `-Project` value from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`, using the workspace install `id`.

## Commands

- `#liveitem status [item_id]`
- `#liveitem clear [item_id|all]`
- `#liveitem clone [new_item_id] [source_item_id]`
- `#liveitem summon [item_id] [charges]`
- `#liveitem bump [item_id] [hp] [mana] [damage]`
- `#itemedit`
- `#itemforge dialog`
- `#itemforge craft type=weapon name=Storm_Blade hp=50 mana=20 ac=5 damage=12 delay=24 haste=5 str=10 heroic_str=2`
- `#itemforge random [count]`
- `#itemforge testbed seed`
- `#itemforge testbed spawn`
- `#itemforge testbed all`
- `#itemforge epicquest seed`
- `#itemforge epicquest spawn`
- `#itemforge epicquest fragments [count]`
- `#helditemid` / `#itemid` / `#cursoritemid` - report the item ID of the cursor-held item for testers.

## Quest API

- Lua `eq.create_live_item({ item_id = ..., data = {...}, modifiers = {...}, custom_data = {...} })`
- Lua `Client:RewardLiveItem(item_inst)`
- Lua `NPC:AddLiveItem(item_inst[, equip])`
- Lua `Corpse:AddLiveItem(item_inst[, slot])`
- Perl `quest::createliveitem({ item_id => ..., data => {...}, modifiers => {...}, custom_data => {...} })`
- Perl `$client->RewardLiveItem($item_inst)`, `$client->PushItemOnCursor($item_inst)`, `$client->PutItemInInventory($slot_id, $item_inst)`
- Perl `$npc->AddLiveItem($item_inst, $equip)` and `$corpse->AddLiveItem($item_inst, $slot)`

## Public Testbed Content

- Tester forge NPC: `900901` Vedra Forgecall, global quest script.
- Tester augment NPC: `900902` Orin Augspinner, global quest script. Testers build a named shardwork augment, spend Blood Shards alternate currency on upgrades, and receive a per-instance augment.
- Instance mutator NPC: `900906` Mavren Instancewright, global quest script plus a tutorialb-local copy so zone defaults cannot capture the public test spawn. Testers hand in exactly one item and receive that same item instance back as a `+1` copy with all supported positive stats raised by 10. NPC id `900903` is reserved for the AI dialogue Sage Aurelian testbed NPC and must not be overwritten by Live Items.
- Evolving heirloom NPC: `900904` Talia Heirloomkeeper, global quest script. Testers request a class-matched heirloom item; `global_player.lua` mutates heirloom instances by +10 to supported positive stats on player level-up.
- Augment fusion NPC: `900905` Nalyx Augmentweaver, global quest script plus a tutorialb-local copy. Testers can say `catalyst` to receive an Augment Catalyst, then hand in one catalyst plus one to three augments and receive one dynamic augment with combined supported stats.
- Tester loot cache: `199091` Live Items Test Loot Cache, global item script.
- Tester loot templates: `199201-199206`; Orin shardwork augment template: `199207`; Nalyx fusion catalyst/test augments/socket item: `199211-199220`; Talia heirlooms use existing low-level weapon base rows and store growth data on the item instance.
- Tutorialb zone script: every normal NPC spawn receives one per-instance live item directly in its loot list for public testbed coverage.

Public testbed promotion applies `002_live_items_testbed_seed.sql` and `004_augment_fusion_testbed_seed.sql`, which remove the older Aelis epic quest test spawn and seed only the tester forge, augment NPCs, cache/template rows, augment fusion rows, and fixed tutorialb tester spawns.

## Native Transport

- `LIVEITEM|ui|open|...`

The server owns item creation and validation. The native window is only an input surface.
