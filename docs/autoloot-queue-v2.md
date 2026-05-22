# AutoLoot Queue V2

This is the target shape for turning AutoLoot into a source-owned loot coordinator instead of a fast corpse processor with a settings window.

Reference: https://www.everquest.com/news/advanced-looting-system

## Product Shape

The player-facing model should be list driven:

- Personal Loot: items assigned to one character. The player can loot, leave, never, or apply saved rules.
- Shared Loot: group or raid items controlled by a master looter or group policy. Items can be assigned, free grabbed, rolled, left, or passed through need/greed/no decisions.
- Rules: item-level saved choices such as Keep, Ignore, Unset, always need, and always greed.
- Automation: optional auto loot all, apply filters, auto show window, coin split, and auto remove looted lore rules.
- Corpse awareness: rows know their corpse/source, locked state, distance/access state, item quantity, and pending decision state.

The native `AoT AutoLoot` window presents the first version of this model today. The source backend emits status, loot rows, and rule rows through `AUTOLOOT|...`; saved Keep rules can still resolve immediately through the safe transfer path.

## Backend Rewrite

Add a source-owned loot queue beside the existing transfer helpers. Do not delete the existing safe transfer path; reuse it when a queued decision becomes a real loot action.

Core objects:

- `AutoLootEntry`: one item or coin bundle from one corpse.
- `AutoLootList`: per-client personal rows and per-group shared rows.
- `AutoLootRule`: saved character or group preference for an item.
- `AutoLootDecision`: pending need/greed/no/assign/free-grab state.
- `AutoLootSnapshot`: compact row data emitted to the native client window and future UI surfaces.

Suggested tables:

- `custom_autoloot_entries`
- `custom_autoloot_character_rules`
- `custom_autoloot_group_rules`
- `custom_autoloot_decisions`
- `custom_autoloot_audit`

Entry state should include:

- entry id, corpse id, corpse entity id, loot slot, item id, item name, quantity, charges
- scope: personal, shared, raid
- owner character id or group id
- state: waiting, locked, rolling, free_grab, assigned, looted, left, expired
- created time and expiry time
- selected recipient and roll winner when applicable

## Server Flow

1. NPC corpse is created normally.
2. Death quests run normally.
3. AutoLootManager inspects loot and coin.
4. If AutoLoot is disabled, keep the current manual corpse behavior.
5. If enabled, coin is handled immediately according to solo/group split rules.
6. Items become personal or shared queue entries instead of being transferred immediately.
7. Saved rules are applied:
   - Ignore leaves or hides the item.
   - Keep attempts immediate transfer for personal rows.
   - Always need/greed marks a roll decision when rolling is active.
   - Auto sell can mark eligible personal rows for sale preview.
8. The manager emits an `AUTOLOOT` snapshot to native clients.
9. Player actions resolve entries through the existing corpse loot transfer path.
10. Failed transfers keep the entry and original corpse item safe.

## Native Bridge

The current `/say #command` bridge is acceptable for the first version. It keeps the server authoritative while the native DLL handles presentation.

Add commands:

```text
#autoloot native snapshot
#autoloot action <entry_id> loot
#autoloot action <entry_id> leave
#autoloot action <entry_id> need
#autoloot action <entry_id> greed
#autoloot action <entry_id> pass
#lootfilter keep <item_id>
#lootfilter ignore <item_id>
#lootfilter unset <item_id>
#autoloot rule <item_id> alwaysneed
#autoloot rule <item_id> alwaysgreed
#autoloot group assign <entry_id> <character>
#autoloot group freegrab <entry_id>
#autoloot group roll <entry_id>
```

Emit snapshots:

```text
AUTOLOOT|snapshot|begin
AUTOLOOT|entry|scope=personal|id=1001|item_id=13073|icon=573|name=Bone Chips|qty=1|source=a skeleton|state=waiting|rule=-
AUTOLOOT|entry|scope=shared|id=1002|item_id=1234|icon=123|name=Example Sword|qty=1|source=Guard Burr|state=rolling|rule=AG
AUTOLOOT|snapshot|end
```

Keep `AUTOLOOT|status|...` as the summary channel.

## UI Target

The native client UI should remain a thin client:

- top toolbar for refresh, loot nearby, autosell, and settings
- Personal Loot table with loot, leave, Keep, and Ignore actions
- Shared Loot table with need, greed, pass, status, and source columns
- settings panel for rules, nearby radius, autosell exclusions, group mode, assigned looter, and recovery tools
- separate rules editor with search, icons, item IDs, and Keep/Ignore/Unset
- no direct item deletion from the client window; every click maps to a server command using an entry id or server-validated item id

## Compatibility Notes

The official later-Live EQ loot window is not natively implemented by this EQEmu/RoF2 code path. This prototype creates its own native SIDL window through the client DLL and keeps the server-side command bridge narrow.

## Migration From Current AutoLoot

Phase 1 shipped the AutoLoot-shaped native client UI.

Phase 2 adds queue entries and snapshots while leaving current immediate processing available as a fallback mode.

Phase 3 routes kill loot, nearby loot, group modes, and no-drop need/greed through entries.

Phase 4 adds saved item rules, free grab, master looter assignment, and roll timers.

Phase 5 connects autosell preview to personal loot rows instead of only bag scans.
