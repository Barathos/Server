# Augs in Augs

Design and operator documentation for the standalone `augs-in-augs` feature.

## Overview

The first prototype is an NPC-driven augment fusion flow, not true recursive augment slots.

Players hand `Nalyx Augmentweaver` exactly one seeded `Augment Catalyst` plus one to three augment items. The NPC consumes the catalyst and augment hand-in, then returns one cloned augment instance whose supported stat fields are the deterministic sum of the handed-in non-catalyst augments. The returned augment keeps the first real augment's base item ID and normal augment type, but stores fused stat overrides in item instance `custom_data` under `dynamic_item.*` keys.

This avoids changing normal socket rules:

- Existing item -> augment behavior remains one level deep.
- No inventory schema change is required for the prototype.
- Fused stats persist through the existing inventory `custom_data` save path.
- The server remains authoritative; clients only receive a resolved item body for display.
- The fusion flow is gated by seeded catalyst ID `499101`; the non-catalyst inputs can be any valid augment item.

## Existing Code Path Notes

- Equipped stat aggregation already reads `ItemInstance::GetItem()` in the normal item/augment bonus path, so a fused augment contributes through the same direct augment path once it is inserted into an item.
- Inventory persistence already serializes item instance `custom_data`; the dynamic stat data rides that path instead of adding nested augment inventory rows.
- RoF/RoF2/SoD/SoF/UF/Titanium item serializers now use `GetClientItem()` for item body data. Dynamic instances use display-only client IDs in the `950000-999998` range so RoF-family item windows do not reuse a stale cached body for the base item ID.
- Client UI cannot naturally show "augments inside augments". This prototype deliberately does not expose nested slots; the NPC produces one normal augment instance with fused stats.

## Test Seed Content

Feature SQL: `features/augs-in-augs/sql/001_augment_fusion_testbed_seed.sql`

- NPC `499100`: `Nalyx_Augmentweaver`, spawned in `tutorialb`.
- Required catalyst: `499101` (`Augment Catalyst`).
- Sample test augments: `499102`, `499103`, `499104`.
- Test socket item: `499110`, with visible type 7 augment slots.

## Local Verification

- Build: `.\verify-feature.ps1 augs-in-augs`
- Install runtime: `.\install-server-runtime.ps1 augs-in-augs`
- Install client files: `.\install-client-files.ps1 augs-in-augs`
- Run DB updates: `.\run-db-updates.ps1 augs-in-augs`
- Validate install: `.\validate-install.ps1 augs-in-augs`

For this prototype SQL, use `-ApplyFeatureSql` when deploying/installing the seed SQL through workspace tooling. Do not apply it to public testbed until explicitly approved.

Manual smoke test after local install:

1. Apply feature SQL and restart/reload quests.
2. In `tutorialb`, find `Nalyx Augmentweaver`.
3. Summon or grant catalyst `499101` and sample augments `499102`, `499103`, and `499104`.
4. Hand Nalyx exactly one `499101` catalyst plus one to three augment items and no coins.
5. Confirm the returned augment item link shows summed supported stats.
6. Insert the fused augment into item `499110` and confirm equipped stats include the fused totals.
7. Camp/relog and confirm the fused item link/stats persist.
8. Try handing augments without a catalyst, two catalysts, coins, or a non-augment source item; the NPC should return the hand-in safely.

## Risks And Limits

- This prototype merges numeric stat fields only. It does not merge click/proc/worn/focus effects, lore groups, scripts, augment distillers, or client-visible nested slots.
- The fused augment's server inventory identity remains the first augment's base item ID; the display body is instance-specific.
- Fusing already fused augments is deterministic because the NPC reads the resolved item stats and writes a new exact total, but balancing rules should be tightened before public use.
- Mixed augment types are allowed by the catalyst gate; the output keeps the first source augment's augment type.
- Dynamic client IDs are display-only. Scripts, persistence, and item checks should continue using the base item ID and custom data markers.

## External Client Sync

- Client patch manifest: `features/augs-in-augs/patcher.yml`
- The first prototype has no custom client files; the patch feed can still generate `eqhost.txt` for testers.
- `-Project` is the workspace install id from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but confirm it first.
- Patcher host commands:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

Missing files are release blockers for real external syncs. Use `-AllowMissingClientFiles` only for partial local testing.
