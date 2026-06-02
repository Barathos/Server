# Augs in Augs Manifest

Feature id: `augs-in-augs`

This manifest starts intentionally small. Add required source files, SQL files,
client UI files, and operator notes as the feature becomes real.

## Test Target

- Server: `D:\EQServers\EQServer-Augs-In-Augs`
- Client: `D:\EQClients\EQClient-Augs-In-Augs`
- Database: `eqemu_augs_in_augs`

## Expected First Implementation Updates

- Dynamic item instance support was ported into `common/item_instance.*`, Lua item instance bindings, and item packet serializers.
- Prototype NPC quest: `features/augs-in-augs/quests/global/499100.lua` (`Augment Catalyst` plus one to three augment inputs).
- Prototype SQL seed: `features/augs-in-augs/sql/001_augment_fusion_testbed_seed.sql` (NPC `499100`, catalyst `499101`, sample augments `499102`-`499104`).
- Focused tests: `tests/dynamic_item_test.h`.
- Add feature-owned files to `features.json` `requiredFiles` before packaging/deploying.
- Add external/test-client files to `features/augs-in-augs/patcher.yml` so the workspace patcher feed can publish them.
- In `patcher.yml`, `source` is repo-relative and `destination` is relative to the EverQuest client root.
- If the feature has native client behavior, add the feature-owned DLL project/source and build this checkout's own `dinput8.dll`.
