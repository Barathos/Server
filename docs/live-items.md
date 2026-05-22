# Live Items

Live item support has two layers:

- Database-backed live item IDs: item IDs in the configured range are resolved from the `items` table at runtime, so new rows and changed rows can be used without `#hotfix`, `#reload`, or a restart.
- Per-instance dynamic item data: item instances can carry custom stat/effect/name overrides in their custom data, so a single player's copy can evolve independently from the base DB row.

## Rules

Defaults are aimed at a test server and reserve `900000-999999` for live items:

```sql
REPLACE INTO rule_values (ruleset_id, rule_name, rule_value, notes)
VALUES
  (1, 'Items:LiveItemLoading', 'true', 'Enable live DB item fallback'),
  (1, 'Items:LiveItemMinID', '900000', 'First live item ID'),
  (1, 'Items:LiveItemMaxID', '999999', 'Last live item ID'),
  (1, 'Items:LiveItemPollIntervalSeconds', '1', 'Live item DB poll interval');
```

Set `Items:LiveItemMinID` and `Items:LiveItemMaxID` to `0` to allow live DB checks for every item ID. Keeping a reserved high range is strongly preferred because normal item IDs stay on the shared-memory path.

## GM Test Commands

```text
#liveitem
#liveitem status 900001
#liveitem clone 900001 1001
#liveitem summon 900001
#liveitem bump 900001 10 10 1
#liveitem clear 900001
#liveitem clear all
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
8. Create a brand-new row externally in the `900000-999999` range, then run `#liveitem summon <new_id>` without restarting.

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
