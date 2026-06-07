# Live Items Testbed Deployment Notes

Project: `live-items` / Live Items / Item Forge  
Repo: `D:\Codex\Apps\EQEmu-feature-live-items`  
Branch: `codex/feature-live-items`  
Matching local target: `D:\EQServers\EQServer-Live-Items`, `D:\EQClients\EQClient-Live-Items`, database `eqemu_live_items`

## Current Status

- Public testbed promotion is working as of commit `2fd3f6646` (`Checkpoint live-items dynamic item display identity`).
- Public patch feed is live at `http://47.181.1.223:8091/patcher/live-items/`.
- Tester patcher executable is `http://47.181.1.223:8091/patcher/live-items/eqemupatcher.exe`.
- Public login/world reachability uses `47.181.1.223:5999` in tester `eqhost.txt` and `47.181.1.223:9000` for world.
- Latest relevant checkpoints:
  - `2fd3f6646` dynamic item client display identity fix.
  - `b810e483f` comprehensive native Item Forge UI.
  - `db4846fd0` feature-owned native client DLL patcher payload.

## Build And Publish Commands

Use the normal project build preset, with Perl pinned explicitly for public testbed verification:

```powershell
cd D:\Codex\Apps\EQEmu-feature-live-items
cmake --preset win-msvc -DEQEMU_BUILD_PERL=ON -DPERL_EXECUTABLE=C:/Strawberry/perl/bin/perl.exe -DPERL_INCLUDE_PATH=C:/Strawberry/perl/lib/CORE -DPERL_LIBRARY=C:/Strawberry/perl/lib/CORE/libperl524.a
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
cmake --build build\win-msvc --config Release --target tests -- /m
.\build\win-msvc\bin\Release\tests.exe
git diff --check
```

Public promotion command:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\publish-testbed-project.ps1 live-items -ApplyServer -ApproveServiceRestart -ApplyFeatureSql
```

Do not use `-RunDatabaseUpdates` for the current Live Items testbed promotion. The feature SQL is standalone under `features/live-items/sql`.

## Fixed Gotchas

- Perl build: use Strawberry Perl 5.24 paths above. Earlier generic CMake runs could miss Perl or bind incorrectly.
- Native DLL ownership: Live Items now owns its feature DLL at `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`. Do not source `dinput8.dll` from `EQEmu-native-client-runtime`.
- Native forge UI: `open forge` requires the Live Items `dinput8.dll` and `EQUI_NativeItemForgeWnd.xml` from this repo's patcher feed.
- Dynamic item inspect cache: RoF/RoF2 clients reused the same item body for multiple per-instance rolls with the same base item ID. Commit `2fd3f6646` adds per-instance client display IDs while preserving the real server item ID.
- Dynamic client display IDs are intentionally kept in the low client-safe reserved chunk `950000-999998`; do not move them into a much higher ID range without in-client RoF2 inspection testing.
- Inventory GUID restore: commit `2fd3f6646` also restores saved inventory `guid` values on load so dynamic item instance identity survives relog.
- Login endpoint: RoF/RoF2 tester clients must use `47.181.1.223:5999` in generated `eqhost.txt`. Do not let patcher generation fall back to `127.0.0.1:5999`. The server-side `serverLoginPort` can remain `5998` for world-to-loginserver local config, but `publicLoginPort` in `D:\Codex\Apps\EQEmu-feature-workspaces\testbed-targets.json` should stay `5999` so the patcher writes the public endpoint.

```ini
[LoginServer]
Host=47.181.1.223:5999
```

Workspace deploy scripts pass `publicLoginHost` and `publicLoginPort` into the patcher generator; keep those pointed at the public endpoint.

## Database And SQL Behavior

Feature SQL applied by `-ApplyFeatureSql`:

- `features/live-items/sql/001_live_items_rules.sql`
- `features/live-items/sql/002_live_items_testbed_seed.sql`
- `features/live-items/sql/003_live_items_log_settings.sql`
- `features/live-items/sql/004_augment_fusion_testbed_seed.sql`

`001_live_items_rules.sql` upserts only `rule_values` for `Items:LiveItemLoading`, `Items:LiveItemMinID`, `Items:LiveItemMaxID`, and `Items:LiveItemPollIntervalSeconds`. The default DB-backed live item range is `150000-199999`; `950000-999998` is reserved for dynamic item client display IDs and should not contain real generated DB rows.

`002_live_items_testbed_seed.sql` is not a full database reset. It deletes/replaces only Live Items test content:

- Deletes old Aelis test content: `npc_types/spawngroup/spawnentry/spawn2` id `900900`.
- Deletes old test item ranges: `items.id BETWEEN 900010 AND 900016`, item `900090`, and `items.id BETWEEN 900101 AND 900116`.
- Replaces alternate currency id `90` mapping to item `81436`.
- Replaces test items `199091` and `199201-199207`.
- Replaces test NPCs/spawns `900901` Vedra Forgecall, `900902` Orin Augspinner, `900904` Talia Heirloomkeeper, and `900906` Mavren Instancewright in `tutorialb`. Live Items must not replace or delete `900903`; that id is reserved for the AI dialogue Sage Aurelian testbed NPC.

It does not intentionally drop or truncate accounts, characters, inventory, bots, guilds, variables, data buckets, or player state. The publish pipeline backs up the remote DB before applying feature SQL; recent backup path pattern:

`003_live_items_log_settings.sql` disables the `CombatRecord` log category outputs so test kills do not spam GM say chat with combat summary lines. It only updates `logsys_categories` where `log_category_description = 'CombatRecord'`.

`004_augment_fusion_testbed_seed.sql` keeps the DB-backed live item range at `150000-199999`, then replaces only the Live Items augment fusion test content: NPC/spawn `900905` and items `199211-199220`. It does not touch accounts, characters, inventory, bots, guilds, variables, data buckets, or other tester state.

```text
D:\EQEmu\Testbed\backups\db-eqemu_live_items-before-YYYYMMDD-HHMMSS.sql.zip
```

## Server Payload

Server code changes include Live Items dynamic item support, live item GM commands, Lua/Perl quest APIs, RoF/RoF2 dynamic instance serialization, and the Item Forge command/native transport.

Feature quest/content files deployed from this repo include:

- `features/live-items/quests/global/900900.lua`
- `features/live-items/quests/global/global_player.lua`
- `features/live-items/quests/global/900901.lua`
- `features/live-items/quests/global/900902.lua`
- `features/live-items/quests/global/900904.lua`
- `features/live-items/quests/global/900905.lua`
- `features/live-items/quests/global/900906.lua`
- `features/live-items/quests/global/items/900090.lua`
- `features/live-items/quests/global/items/199091.lua`
- `features/live-items/quests/tutorialb/900905.lua`
- `features/live-items/quests/tutorialb/900906.lua`
- `features/live-items/quests/tutorialb/zone.lua`
- `features/live-items/sql/001_live_items_rules.sql`
- `features/live-items/sql/002_live_items_testbed_seed.sql`
- `features/live-items/sql/003_live_items_log_settings.sql`
- `features/live-items/sql/004_augment_fusion_testbed_seed.sql`

Public testbed content:

- `900901` Vedra Forgecall: Item Forge native UI tester.
- `900902` Orin Augspinner: selectable-stat Blood Shard augment tester.
- `900904` Talia Heirloomkeeper: class-matched heirloom issuer; `global_player.lua` grows marked heirlooms by +10 supported positive stats on `event_level_up`.
- `900905` Nalyx Augmentweaver: hand in one Augment Catalyst plus one to three augments to receive a single fused dynamic augment with combined supported stats. `features/live-items/quests/tutorialb/900905.lua` mirrors the global script for the public test spawn.
- `900906` Mavren Instancewright: hand-in one item to mutate that exact instance into a `+1` copy with supported positive stats raised by 10. `features/live-items/quests/tutorialb/900906.lua` intentionally mirrors the global script for the public test spawn because zone-local NPC ID scripts resolve before `tutorialb/default.pl`.
- `199091` Live Items Test Loot Cache.
- `199201-199206` random loot templates.
- `199207` Orin shardwork augment template.
- `199211-199220` Nalyx fusion catalyst, test augments, and socket item.
- Talia's evolving heirlooms use existing low-level weapon base item rows and store class/name/stat growth on the item instance.
- `tutorialb` zone script adds one per-instance live item directly to normal NPC loot on spawn for test coverage.

Server restart is required after publishing server binaries. Use `-ApproveServiceRestart` with `publish-testbed-project.ps1`.

## Client Patcher Payload

Source of truth: `features/live-items/patcher.yml`.

Current client install list:

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` -> `dinput8.dll`
- `client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml` -> `uifiles/default/EQUI_NativeItemForgeWnd.xml`
- generated `eqhost.txt`
- generated `uifiles/default/EQUI.xml` with `EQUI_NativeItemForgeWnd.xml` included

The feed also deletes stale native UI windows from other features, including AutoLoot, Achievements, Spell Forge, Multiclass, MQ Interface, Tradeskills, and HP Fix XML files. This is intentional for isolated feature test clients.

Regenerate/test only the patcher feed when server files do not need redeploy:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\publish-testbed-project.ps1 live-items -SkipServerDeploy
```

Manual patcher generation path if needed:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project live-items -BaseUrl http://47.181.1.223:8091/patcher/ -LoginHost 47.181.1.223 -LoginPort 5999
.\Test-WorkspacePatcherDeployment.ps1 -Project live-items -BaseUrl http://47.181.1.223:8091/patcher/
```

Missing files in `patcher.yml` are release blockers for external tester syncs. Use `-AllowMissingClientFiles` only for partial local testing.

## Rollback And Recovery

- Server package exports are under `D:\Codex\Apps\EQEmu-feature-workspaces\exports\testbed\live-items-<commit>-<timestamp>\`.
- Remote DB backups are under `D:\EQEmu\Testbed\backups\`.
- Remote patcher publish backups use `D:\EQEmu\Testbed\backups\patcher-publish-before-YYYYMMDD-HHMMSS.zip`.
- Remote testbed server root is `D:\EQEmu\Testbed\server`.
- Remote patcher publish root is `D:\EQEmu\Testbed\patcher\publish`.

Rollback options:

1. Re-run `publish-testbed-project.ps1` from a known-good commit.
2. Restore a `db-eqemu_live_items-before-*.sql.zip` backup if feature SQL damaged test content.
3. Restore `patcher-publish-before-*.zip` if patcher feed output is wrong.

## Copy / Avoid / Verify For Other Project Chats

- Copy: project-local `features/<feature-id>/patcher.yml` ownership of all external client payloads.
- Copy: explicit Perl pinning when building/publishing Windows testbed server binaries.
- Copy: target public login verification after patcher feed generation: inspect `rof/eqhost.txt` and test the advertised login port.
- Avoid: depending on another feature's dirty DLL or `EQEmu-native-client-runtime` for feature-specific client behavior.
- Avoid: `-RunDatabaseUpdates` unless the feature added compiled world database updates.
- Avoid: broad SQL deletes during testbed seed. Keep deletes scoped to feature-owned high ID ranges.
- Verify before promotion: clean worktree, `zone`/`world` builds, tests where relevant, `git diff --check`, patcher feed file list, `eqhost.txt`, and server restart success.
