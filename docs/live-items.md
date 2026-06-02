# Live Items

Live item support has two layers:

- Database-backed live item IDs: item IDs in the configured range are resolved from the `items` table at runtime, so new rows and changed rows can be used without `#hotfix`, `#reload`, or a restart.
- Per-instance dynamic item data: item instances can carry custom stat/effect/name overrides in their custom data, so a single player's copy can evolve independently from the base DB row.

## Rules

Defaults are aimed at a test server and reserve `150000-199999` for DB-backed live item rows:

```sql
REPLACE INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES
  (1, 'Items:LiveItemLoading', 'true', 'Enable live DB item fallback'),
  (1, 'Items:LiveItemMinID', '150000', 'First live item ID'),
  (1, 'Items:LiveItemMaxID', '199999', 'Last DB-backed live item ID'),
  (1, 'Items:LiveItemPollIntervalSeconds', '1', 'Live item DB poll interval');
```

Set `Items:LiveItemMinID` and `Items:LiveItemMaxID` to `0` to allow live DB checks for every item ID. Keeping a reserved feature range is strongly preferred because normal item IDs stay on the shared-memory path.

Per-instance dynamic items use display-only client item IDs in the reserved `950000-999998` chunk to avoid RoF2 item-window cache reuse. Keep real generated Item Forge DB rows out of that display-only subrange.

## GM Test Commands

```text
#liveitem
#liveitem status 900001
#liveitem clone 900001 1001
#liveitem summon 900001
#liveitem bump 900001 10 10 1
#liveitem clear 900001
#liveitem clear all
#itemforge epicquest seed
#itemforge epicquest spawn
#itemforge epicquest fragments 5
```

`clone` creates or replaces a DB row by copying an existing source item into the live range, then summons it. `bump` edits HP, mana, and damage and updates the row timestamp, but intentionally does not clear the cache; wait up to `Items:LiveItemPollIntervalSeconds` and the held item should refresh through the automatic path.

## Cursor Item Editing

GMs can hold any item on the cursor and run:

```text
#itemedit
#itemedit add hp 25
#itemedit set name Vexen's Slightly Unsafe Sword
#itemedit proc 1234 150 1
#itemedit clear hp
#itemedit clear all
```

`#itemedit` writes per-instance dynamic item data, not the base `items` row. The menu prints current and base values plus clickable +/- links for numeric fields, then saves and refreshes the cursor item after each edit.

## Test Checklist

1. Start world and zone normally.
2. Log in a GM character.
3. Run `#liveitem clone 900001 <known_source_item_id>`.
4. Inspect the summoned item and confirm the generated name and stat bumps.
5. Run `#liveitem bump 900001 25 25 2`.
6. Wait one poll interval, then inspect the held item again. Equipped items should also send a charm update and recalculate bonuses when the live refresh sees changed data.
7. Hold the item on your cursor and run `#itemedit add hp 25`, then inspect the cursor item.
8. Create a brand-new row externally in the `150000-199999` range, then run `#liveitem summon <new_id>` without restarting.

## Public Testbed Harness

The current public tester round is intentionally focused on live item mechanics rather than the Aelis class epic quest. Public testbed promotion applies `features/live-items/sql/002_live_items_testbed_seed.sql` and `features/live-items/sql/004_augment_fusion_testbed_seed.sql`, which remove the older Aelis test spawn and seed only:

- `900901` Vedra Forgecall for Item Forge UI testing.
- `900902` Orin Augspinner for selectable-stat Jagged Blood Shard augment testing.
- `900906` Mavren Instancewright for hand-in instance mutation testing. Hand in exactly one item and it returns as a `+1` copy with supported positive stats increased by 10. If the item has no supported stats yet, Mavren seeds HP, mana, endurance, and AC so testers can still see the instance mutation. Live Items intentionally avoids `900903`, which is reserved for the AI dialogue Sage Aurelian testbed NPC.
- `900904` Talia Heirloomkeeper for evolving item testing. Say `heirloom` to receive a class-matched per-instance weapon; the global player level-up hook grows that exact heirloom by +10 to supported positive stats each level.
- `900905` Nalyx Augmentweaver for augment fusion testing. Hand in one Augment Catalyst plus one to three augments and receive a single dynamic augment with the supported stats combined.
- `199091` Live Items Test Loot Cache for command/NPC-driven cache rolls.
- `199201-199206` tester base templates.
- `199211-199220` augment fusion catalyst, test augments, and socket test item.
- Evolving heirlooms use existing low-level weapon bases and put the unique name, class mask, stats, and growth marker on the item instance.
- Fixed `tutorialb` spawns for Vedra, Orin, Mavren, Talia, and Nalyx.

The `tutorialb` zone script adds one per-instance live item directly to each normal NPC's loot list on spawn, so corpse loot tests do not require cache-opening or a separate quest.

## Quest Script API

Curated quests can build a per-instance live item from one compact spec. The base `item_id` is reused; `data` applies exact dynamic item fields, `modifiers` applies additive stats, and `custom_data` stores quest metadata on that one item instance.

Lua:

```lua
local inst = eq.create_live_item({
  item_id = 900001,
  charges = 1,
  data = {
    name = e.other:GetCleanName() .. "'s Forged Blade",
    lore = "A quest-forged live item.",
    hp = 100,
    mana = 100,
    ac = 15,
    proc_effect = 1234,
    proc_type = 0,
    proc_level = 1,
    proc_level2 = 255,
    proc_rate = 150,
  },
  modifiers = {
    str = 12,
    dex = 8,
    fr = 10,
  },
  custom_data = {
    quest = "forge_intro",
    roll = "STR +12, DEX +8, FR +10",
  },
})

e.other:RewardLiveItem(inst)
```

Perl:

```perl
my $inst = quest::createliveitem({
  item_id => 900001,
  charges => 1,
  data => {
    name => "$name's Forged Blade",
    lore => "A quest-forged live item.",
    hp => 100,
    mana => 100,
    ac => 15,
  },
  modifiers => {
    str => 12,
    dex => 8,
    fr => 10,
  },
  custom_data => {
    quest => "forge_intro",
    roll => "STR +12, DEX +8, FR +10",
  },
});

$client->RewardLiveItem($inst);
```

`RewardLiveItem` places the built instance on the cursor. Perl also exposes `$client->PushItemOnCursor($inst)` and `$client->PutItemInInventory($slot_id, $inst)` for direct placement. Lua already exposes `PushItemOnCursor` and `PutItemInInventory`, with `RewardLiveItem` as the matching curated quest helper.

Supported spec keys are `item_id`, `charges`, `augment_one` through `augment_six`, `attuned`, `data` or `set`, `modifiers` or `mods`, `custom_data` or `custom`, and optional `rebuild = false` if the quest wants to batch more changes before calling `RebuildDynamicItemData()`.

## Client Patcher Sync

Client-facing Live Items files are synced through `features/live-items/patcher.yml`. Do not use the local EQ client folder as the source of truth. Add the file to this repo first, then add a manifest entry mapping the repo path to the EverQuest client destination:

```yaml
files:
  - source: client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml
    destination: uifiles/default/EQUI_NativeItemForgeWnd.xml

generated:
  eqhost: true
  equiXml: true
  equiIncludes:
    - EQUI_NativeItemForgeWnd.xml
```

In `patcher.yml`, `source` is always a path inside this repo and `destination` is always relative to the EverQuest client root. Set `generated.eqhost = true` when the patcher should write `eqhost.txt`, set `generated.equiXml = true` when native UI XML includes need to be injected, and list custom `EQUI_*.xml` windows explicitly in `generated.equiIncludes`.

Missing files in `patcher.yml` are release blockers for real external/test-client syncs. Use `-AllowMissingClientFiles` only for partial local testing.

Regenerate and test the project feed from the patcher project:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
```

Resolve `<project-id>` from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`; it is the workspace install `id`, which usually matches the feature id but should not be assumed blindly. The feed publishes at `http://<patch-host>:8091/patcher/<project-id>/`. External testers place that project's generated `eqemupatcher.exe` in their EQ client root and run it.

## Lua Dynamic Item Example

This creates a unique player copy from an existing base item without needing a new DB row:

```lua
function event_say(e)
  if e.message:findi("forge") then
    local inst = ItemInst(900001, 1)
    inst:SetDynamicItemData("name", e.other:GetCleanName() .. "'s Forged Blade")
    inst:SetDynamicItemModifier("hp", 50)
    inst:SetDynamicItemModifier("mana", 25)
    inst:SetDynamicItemModifier("agi", 7)
    inst:SetDynamicItemData("haste", 18)
    inst:SetDynamicItemData("proc_effect", 1234)
    inst:SetDynamicItemData("proc_type", 0)
    inst:SetDynamicItemData("proc_level", 1)
    inst:SetDynamicItemData("proc_level2", 255)
    inst:SetDynamicItemData("proc_rate", 150)
    e.other:PushItemOnCursor(inst)
  end
end
```

On a level-up or quest milestone, mutate the held instance and send a live item update:

```lua
function event_level_up(e)
  local inst = e.other:GetInventory():GetItem(13) -- primary slot
  if inst and inst:GetID() == 900001 then
    inst:SetDynamicItemModifier("hp", e.other:GetLevel() * 10)
    inst:SetDynamicItemModifier("mana", e.other:GetLevel() * 5)
    e.other:SendItemScale(inst)
  end
end
```

Dynamic fields are stored in item custom data, so they persist with that specific item instance. Use absolute setters for exact values and modifiers for additive stats.

## Evolving Item Test Flow

`900904` Talia Heirloomkeeper demonstrates item-instance growth without creating new item IDs. The NPC creates a class-matched heirloom from an existing low-level weapon base row, writes the class usability, name, damage, delay, and starter stats onto that specific item instance, marks it with `live_items_heirloom = 1`, and gives it to the player. `features/live-items/quests/global/global_player.lua` listens for `event_level_up`, scans equipped and inventory slots for marked heirlooms, clones the found instance, raises supported positive stats by 10 per level gained, writes the new values with dynamic item data, then saves the updated instance back to the same slot.

`900906` Mavren Instancewright demonstrates quest hand-in mutation. The player hands in exactly one item, the script clones that handed-in instance, converts supported current positive stats into exact dynamic values at `current + 10`, consumes the original hand-in, and returns the upgraded instance on the cursor. This proves quests can preserve and mutate one exact rolled copy rather than recreating a generic item by ID.

## Augment Fusion Test Flow

`900905` Nalyx Augmentweaver demonstrates combining item instances without true nested augments. The player hands in exactly one Augment Catalyst (`199211`) plus one to three augment items. The quest validates that every non-catalyst hand-in is an augment, clones the first augment instance, clears old dynamic item data from the clone, writes summed supported stats as dynamic item data, marks the result with fusion custom data, consumes the source items, and returns one fused augment on the cursor.
