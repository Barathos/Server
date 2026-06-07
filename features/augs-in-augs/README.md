# Augs in Augs

Standalone EQEmu feature branch for `augs-in-augs`.

## Test Target

- Server: `D:\EQServers\EQServer-Augs-In-Augs`
- Client: `D:\EQClients\EQClient-Augs-In-Augs`
- Database: `eqemu_augs_in_augs`

## First Build Loop

From `D:\Codex\Apps\EQEmu-feature-workspaces`:

~~~powershell
.\verify-feature.ps1 augs-in-augs
.\install-server-runtime.ps1 augs-in-augs
.\install-client-files.ps1 augs-in-augs
.\run-db-updates.ps1 augs-in-augs
.\validate-install.ps1 augs-in-augs
~~~

`install-server-runtime.ps1` also refreshes Windows firewall allow rules for the copied server binaries.

## Client UI

The first prototype is server-side only. No native client DLL or XML window is required because the RoF2 client cannot naturally display nested augment slots; fused augments are ordinary augment items with server-authored instance stats.

## Client Patcher Feed

External/test-client patch syncing is owned by `features/augs-in-augs/patcher.yml`. Do not add files directly to `D:\EQClients\EQClient-Augs-In-Augs` as the source of truth. Add any client-facing XML, DLL, config, zone asset, patch note, status file, or other tester-facing file to this repo and list it there before publishing a patcher feed.

## Development Notes

The first implementation direction is NPC-driven augment fusion rather than true nested augment sockets.

- `features/augs-in-augs/quests/global/499100.lua` owns the prototype NPC hand-in flow.
- `features/augs-in-augs/sql/001_augment_fusion_testbed_seed.sql` seeds Nalyx Augmentweaver, `Augment Catalyst` ID `499101`, and test augments/items below `500000`.
- Fused augment stats are stored as per-instance dynamic item data in normal inventory `custom_data`.
- Normal augment insertion/removal behavior is unchanged.
- The NPC requires exactly one catalyst plus one to three valid augment items, so the feature is opt-in without whitelisting every possible augment.
- The client does not show nested augment slots; the returned augment is a normal augment with server-authored fused stats.

Use `-ApplyFeatureSql` only when intentionally applying the standalone seed SQL. Do not promote/apply it to public testbed without explicit approval.
