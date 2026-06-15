# Live Items Cloud Handoff

Date: 2026-06-15
Project: `EQEmu-feature-all`
Branch: `codex/features-all`
Owned remote: `origin = https://github.com/Barathos/Server.git`

## Goal

Build the next Live Items implementation pass in the all-features project using
the backend direction captured in `docs/live-items-backend-plan.md`.

The coding target is not a final loot design yet. The target is the reusable
backend that can support randomized prefix/suffix items, curated quest rewards,
upgrade NPCs, rolled augments, optional Item Forge crafting, and GM recovery.

## Current Direction

Live Items should use reusable template item IDs and per-instance item metadata.

- Template item IDs are archetypes, not unique item identities.
- Generated player items should store frozen final stats/name/effects in
  instance `custom_data`.
- Normal generated items should not require permanent generated rows in the
  `items` table.
- Existing generated DB-row commands remain useful as GM/operator tools.
- Player-owned generated items should be searchable and recoverable by GM tools.
- Rolled augments need instance-aware duplicate checks so two different rolls
  from the same augment template can coexist when rules allow it.

## First Implementation Milestone

The first milestone should prove operational safety, not content balance.

1. Standardize generated item metadata, especially `live_items.instance_id`.
2. Add registry/audit persistence for generated item instances.
3. Add GM inspect/explain/search/restore commands.
4. Convert one safe generator path to create per-instance template items.
5. Add instance-aware rolled augment duplicate logic.
6. Add tests for persistence, clone/restore, and duplicate augment policy.

Do not make random loot broadly player-facing until the movement and recovery
paths are proven.

## Suggested GM Command Shape

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

## Rolled Augment Requirement

Current EQEmu duplicate augment checks are base-item-ID based. That breaks rolled
Live Items augments, because two different rolled augments from one template are
treated as the same augment.

The intended replacement is an equivalent-augment check:

```text
normal augment:
  duplicate key = base item_id

rolled live augment:
  duplicate key = live_items.instance_id

restricted rolled augment family:
  duplicate key = live_items.augment_exclusive_key
```

Do not solve this by globally enabling `Inventory:AllowMultipleOfSameAugment`.

## Existing Server Overlay Test

Keep this as a future validation gate. The feature should be safe to layer onto
a running server that does not yet have generated Live Items, as long as support
is deployed before generation is enabled.

Test sequence:

1. Deploy schema and code with generation disabled.
2. Verify old normal items still work through login, zone, bank, trade, inspect,
   death, and corpse recovery.
3. Enable GM-only generation.
4. Generate template-based prefix/suffix items.
5. Move generated items through cursor, equipment, bags, bank, shared bank,
   trade, corpse, and relog.
6. Search generated items by instance ID, character, base template, affix, name,
   and source.
7. Restore a deliberately deleted/lost generated item from registry data.
8. Insert two different rolled augments from the same augment template and verify
   duplicate policy behavior.

## Cloud Work Notes

Cloud coding is appropriate for C++, SQL, docs, command work, and unit tests.
Final validation still needs Windows/MSVC coverage for this repository.

Before starting cloud work, ensure the local branch has been pushed to
`origin/codex/features-all`. This local checkout currently has unrelated
Advanced Loot worktree changes, so do not assume every uncommitted file belongs
to Live Items.

Use `docs/live-items-backend-plan.md` as the authoritative direction document
for this implementation pass.
