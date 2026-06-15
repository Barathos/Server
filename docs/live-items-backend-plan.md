# Live Items Backend Direction

Date: 2026-06-15

This document captures the current design direction for Live Items before the
final live-server content model is chosen. The feature should provide backend
support for randomized prefix/suffix items, curated quest rewards, upgrades,
rolled augments, and optional Item Forge-style creation without requiring one
new `items` table row per player-owned item.

## Current Vision

Live Items should be a safe instance-item backend first and a specific loot
system second.

The live-server vision may later become Diablo-style randomized loot, simpler
prefix/suffix drops, quest-built signature rewards, Item Forge crafting, augment
fusion, upgrade NPCs, or a mix of those. The backend should support all of those
without committing the server to one player-facing loop too early.

The current preferred player-facing promise is:

- A generated item is a real, inspectable, trade-safe item.
- A generated item survives zone, relog, death/corpse recovery, bank/shared bank,
  trade, and normal inventory movement.
- A generated item does not silently change because an operator later edits its
  base template row.
- Operator tooling can find, inspect, explain, clone, and restore generated item
  instances.

## Core Model

Use normal item IDs as templates or archetypes, not as unique generated item
identity.

```text
items row          = reusable base template/archetype
inventory row      = actual owned item instance
custom_data        = frozen generated identity, affixes, stats, and metadata
registry/audit row = searchable GM index and restore snapshot
```

The existing per-instance dynamic item path is the correct foundation. A rolled
item should normally keep the template's `item_id`, then store its generated
name, stat overrides, affixes, source, and identity in item instance
`custom_data`.

Do not use permanent generated DB rows as the normal player-facing path. DB
live item rows remain useful for GM testing, operator hot-edit tools, and
possibly spell/item prototype work, but they should not be the default way to
represent thousands of player-owned randomized items.

## Suggested Instance Metadata

Generated items should stamp enough metadata to support GM search and recovery.
Suggested custom data keys:

```text
live_items.instance_id
live_items.template_id
live_items.roll_seed
live_items.source
live_items.created_at
live_items.created_by
live_items.created_for_character_id
live_items.created_for_account_id
live_items.rarity
live_items.prefix_id
live_items.suffix_id
live_items.affix_summary
live_items.version
```

Dynamic item fields should store the final frozen result:

```text
dynamic_item.set.name
dynamic_item.set.lore
dynamic_item.set.hp
dynamic_item.set.mana
dynamic_item.set.ac
dynamic_item.set.damage
dynamic_item.set.proc_effect
...
```

Use exact `dynamic_item.set.*` values for generated final stats when the item
must be stable forever. Use `dynamic_item.mod.*` only when the design
intentionally wants the item to layer on top of a later-updated base template.

The current default policy for player-owned generated items is freeze-at-
creation: later edits to the base template should not silently rebalance already
owned items unless a specific migration or upgrade tool intentionally rewrites
the instance.

## Registry And GM Recovery

Per-instance items can exist without a unique DB row in `items`, but live
operation needs a searchable index. Add a registry/audit table rather than
turning every rolled item into a permanent item row.

Suggested table:

```text
custom_live_item_instances
- instance_id
- base_item_id
- current_character_id
- current_account_id
- current_location
- current_slot
- display_name
- rarity
- prefix_id
- suffix_id
- roll_seed
- source
- custom_data_snapshot
- created_at
- updated_at
- deleted_at
- lost_at
```

The registry is not the source of truth for normal item ownership. Normal
inventory, shared bank, corpse, bot inventory, parcel, and trade systems still
carry the item. The registry is an operator index and restore source. If a
player deletes or loses an item and no inventory/snapshot row remains, the GM
can recreate it from `base_item_id + custom_data_snapshot`.

The first production-grade GM tools should include:

```text
#liveitem instance inspect cursor
#liveitem instance explain cursor
#liveitem instance search name <text>
#liveitem instance search char <name>
#liveitem instance search base <item_id>
#liveitem instance search affix <prefix_or_suffix>
#liveitem instance search guid <guid_or_instance_id>
#liveitem instance restore <instance_id> <character>
#liveitem instance clone cursor <character>
```

Operator output should show base item ID, current resolved name, live instance
ID, source, rarity, affixes, current known owner/location, and a compact stat
summary.

## Prefix And Suffix Support

The backend should allow multiple generation strategies:

- Static affix: a prefix or suffix always applies the same stat package.
- Rolled affix: a prefix or suffix rolls values from configured ranges.
- Curated quest affix: a quest or NPC applies exact named stats.
- Upgrade affix: an NPC mutates an existing instance and records rank/history.
- Hybrid roll: templates, rarity, prefix, suffix, and source all contribute.

Regardless of strategy, the generated item should store the final result on the
instance. The affix IDs and roll seed explain how it was produced, but the
player-owned item should not depend on live affix table values remaining
unchanged forever.

## Item Forge Role

Item Forge should be treated as optional content on top of the same instance
backend.

Possible final roles:

- GM/test tool for creating and inspecting item instances.
- Player crafting UI that creates per-instance items from safe templates.
- Progression system with costs, limits, rarity rules, and affix pools.

The current DB-row-creating Item Forge path is useful for proving UI and command
transport, but it is not the preferred long-term production path for randomized
player items. A production Item Forge should create per-instance items from
templates unless there is an explicit operator reason to create a new DB row.

## Rolled Augments

Rolled augments need special handling. EQEmu currently blocks multiple augments
with the same base item ID in one item unless `Inventory:AllowMultipleOfSameAugment`
is globally enabled. That is too broad for Live Items.

For rolled Live Items augments, duplicate checks should compare a Live Items
augment identity instead of only the template item ID.

Suggested duplicate key rules:

```text
normal augment:
  duplicate key = base item_id

rolled live augment:
  duplicate key = live_items.instance_id

restricted rolled augment family:
  duplicate key = live_items.augment_exclusive_key
```

Suggested metadata:

```text
live_items.instance_id
live_items.template_id
live_items.augment_duplicate_policy = instance|template|exclusive_key
live_items.augment_exclusive_key
```

This allows two different rolled augments from the same template to fit in the
same item when intended, while still allowing design rules such as "only one
Firecore augment per item."

Do not enable `Inventory:AllowMultipleOfSameAugment` globally as the normal fix.
That weakens stock augment behavior everywhere.

## Safe Rollout Onto Existing Servers

This feature can be layered onto an existing running server without converting
old items, provided the instance plumbing is deployed first and generation is
kept disabled until validated.

Existing items with empty `custom_data` should continue to behave like stock
items. New Live Items are normal base item IDs plus instance metadata and
dynamic fields.

Safe rollout order:

1. Deploy schema and code support with generation disabled.
2. Verify existing characters can log in, zone, bank, trade, inspect items, die,
   and recover corpses normally.
3. Enable GM-only generation on a test character.
4. Generate template-based instance items and test relog, zone, trade, bank,
   shared bank, corpse recovery, and inspection.
5. Enable controlled NPC/quest/loot generation only after persistence paths are
   proven.

The main migration risk is not old item breakage. The main risk is a missed
persistence or movement path stripping `custom_data`, which would turn a rolled
item back into its plain template. The first live milestone must prove instance
data survives every item movement path.

## Future Test Notes

Before calling Live Items production-ready, run an explicit "currently running
server overlay" test:

1. Start from a server/database with no player-owned generated Live Items.
2. Deploy Live Items schema/code with generation disabled.
3. Run baseline regression on existing normal items.
4. Enable GM-only generation and create prefix/suffix template items.
5. Move generated items through cursor, equipment, general inventory, bags, bank,
   shared bank, trade, corpse, and relog.
6. Search for the generated item by instance ID, character, base template,
   affix, name, and source.
7. Delete or lose a generated item intentionally and restore it from the
   registry snapshot.
8. Insert two different rolled augments from the same augment template into the
   same item and verify the Live Items duplicate policy allows or blocks them as
   configured.
9. Confirm old normal items remain unaffected throughout the test.

## Implementation Phases

Phase 1: Stabilize instance identity.

- Add or standardize `live_items.instance_id` stamping.
- Ensure generated items carry source, template, affix, and version metadata.
- Prefer frozen `dynamic_item.set.*` results for player-owned generated items.

Phase 2: Add registry and GM tooling.

- Add `custom_live_item_instances` or equivalent.
- Index generated item creation/update/delete/restore events.
- Add inspect, explain, search, clone, and restore commands.

Phase 3: Convert generators to the instance backend.

- Keep DB-row Live Item commands as GM/operator tools.
- Update Item Forge, quest rewards, corpse rolls, heirlooms, upgrades, and augment
  fusion to create or mutate instances from templates.

Phase 4: Fix rolled augment duplicate identity.

- Add instance-aware augment duplicate helpers.
- Replace base-ID-only duplicate checks in client augment insertion and tradeskill
  augment paths.
- Preserve stock EQ behavior for normal augments.

Phase 5: Choose the live content model.

- Decide which loops are actually player-facing: random corpse drops, prefix/
  suffix loot, Item Forge, upgrades, heirlooms, augment fusion, quest rewards, or
  some limited combination.
- Tune affix pools, rarity, costs, and drop sources after the backend is proven.
