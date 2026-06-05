# All Features Bundle

This branch is the maintained source of truth for the combined custom EQEmu
distribution on top of a clean EQEmu base:

- AutoLoot
- Live Items / Item Forge
- Live Spells / Spell Forge
- Achievements
- Multiclass
- Item Rarity
- Native Interface / Gearscore
- Tradeskills
- HP Fix
- EQEMU General Code
- AI NPC Response
- Augs in Augs
- Dynamic Quests

## Branch Shape

The all-features checkout now owns the maintained combined package. Standalone feature
projects are useful as historical references or extraction targets, but normal feature
fixes for the combined server should land here and be protected by runtime feature
gates instead of maintaining separate live implementations.

Feature gates live in the `CustomFeatures` rule category:

- `CustomFeatures:MulticlassEnabled`
- `CustomFeatures:AchievementsEnabled`
- `CustomFeatures:AutoLootEnabled`
- `CustomFeatures:GearScoreEnabled`
- `CustomFeatures:LiveItemsEnabled`
- `CustomFeatures:LiveSpellsEnabled`
- `CustomFeatures:ItemRarityEnabled`
- `CustomFeatures:HpFixEnabled`
- `CustomFeatures:AiDialogueEnabled`
- `CustomFeatures:TradeskillsEnabled`
- `CustomFeatures:AugsInAugsEnabled`
- `CustomFeatures:DynamicQuestsEnabled`
- `CustomFeatures:MqInterfaceEnabled`

Use `#customfeatures` in-game as a GM admin to print the current all-features gate
state. When adding or changing feature behavior, gate it at server authority
boundaries: commands, passive event processors, native transport senders, quest entry
points, and item/spell mutation paths. The all-features client patcher may still ship
all XML and native DLL payloads together; disabled server rules should prevent the
feature from doing real work.

## Database

The combined custom database manifest uses:

- Custom version `1`: AutoLoot schema.
- Custom version `2`: Achievement schema.
- Custom version `3`: Achievement catalog seed.
- Custom version `4`: Live hunter achievement seed.
- Custom version `5`: Gearscore item power schema.
- Custom version `6`: Item rarity schema.
- Custom version `7`: Multiclass schema.

`common/version.h` sets `CUSTOM_BINARY_DATABASE_VERSION` to `7`.

## Native Client Assets

This bundle includes one bundle-owned native DLL runtime plus feature-owned XML windows.
Do not source `dinput8.dll` from another feature checkout or from
`EQEmu-native-client-runtime`; this project builds and publishes its own all-features
client DLL.

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`
- `client_files/native_autoloot/eq-core-dll/`
- `client_files/native_autoloot/config/native_interface.ini`

- `EQUI_NativeAutoLootWnd.xml`
- `EQUI_NativeItemForgeWnd.xml`
- `EQUI_NativeSpellForgeWnd.xml`
- `EQUI_NativeAchievementWnd.xml`
- `EQUI_NativeMulticlassWnd.xml`
- `EQUI_NativeTradeskillsWnd.xml`
- `EQUI_NativeHpFixWnd.xml`
- `EQUI_NativeAugsInAugsWnd.xml`
- `EQUI_NativeDynamicQuestsWnd.xml`

The runtime handles `AUTOLOOT|`, `LIVEITEM|`, `LIVESPELL|`, `ACH|`, `HPFIX|`,
`ITEMPOWER|`, and `ITEMRARITY|` transport lines. It also owns the native
interface module for map commands, native interface diagnostics, and
Gearscore/ItemPower ItemDisplay decoration. It is no longer only lab code,
but it is still monolithic internally: most feature-specific client behavior
lives in `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h`
or `client_files/native_autoloot/eq-core-dll/src/native_interface.cpp`.
The next cleanup is splitting that into a reusable native-client base plus
feature-specific native modules.

## AI NPC Response

The AI NPC Response prototype is included as a feature payload under
`features/ai-npc-response/`. It contains a localhost-only FastAPI/Ollama bridge,
a Tutorial B Perl quest for Sage Aurelian, and an idempotent SQL seed for NPC
and spawn id `900903`.

This prototype currently has no client-facing files. Its standalone
`features/ai-npc-response/patcher.yml` is intentionally empty, so the
all-features client patcher does not publish an AI NPC native XML window until
that feature commits one as source of truth.

The expanded all-features target stages additional feature payloads in this checkout
first, then wires server/native behavior here. Standalone feature projects remain in
their current state; this checkout owns the combined package.

## Live Items Testbed Payload

The public-testbed Live Items NPC scripts and seed SQL are mirrored under
`features/all-features/` so the bundle package can publish them as all-features-owned
content. The source copies came from the Live Items checkpoint `079c7de44`.

- `features/all-features/quests/global/900901.lua`
- `features/all-features/quests/global/900902.lua`
- `features/all-features/quests/global/900904.lua`
- `features/all-features/quests/global/900905.lua`
- `features/all-features/quests/global/900906.lua`
- `features/all-features/quests/global/global_player.lua`
- `features/all-features/quests/global/items/199091.lua`
- `features/all-features/quests/tutorialb/900905.lua`
- `features/all-features/quests/tutorialb/900906.lua`
- `features/all-features/quests/tutorialb/zone.lua`
- `features/all-features/sql/001_live_items_rules.sql`
- `features/all-features/sql/002_live_items_testbed_seed.sql`
- `features/all-features/sql/003_live_items_log_settings.sql`
- `features/all-features/sql/004_augment_fusion_testbed_seed.sql`

Do not add a Live Items `900903` quest. NPC and spawn id `900903` are reserved for
Sage Aurelian / AI NPC Response and should be preserved in place during testbed
promotion.

## Client Patcher

Client patch syncing for this bundle is owned by `features/all-features/patcher.yml`.
Any file that external testers or the matching test client need should be added to
this repo first, then listed in that manifest. Do not add files directly to the
local EQ client as the source of truth.

In `patcher.yml`:

- `source` is the path inside this repo.
- `destination` is the path inside the EverQuest client root.
- `generated.eqhost: true` tells the patcher to write `eqhost.txt`.
- `generated.equiXml: true` tells the patcher to inject native UI XML includes.
- `generated.equiIncludes` should list custom `EQUI_*.xml` windows explicitly.

Current patcher manifest responsibilities:

- Publish `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` to `dinput8.dll`.
- Publish `client_files/native_autoloot/config/native_interface.ini` to `native_interface.ini`.
- Publish all native XML windows to `uifiles/default/`.
- Generate `eqhost`.
- Generate `EQUI.xml` with includes for every custom native window listed in
  `features/all-features/patcher.yml`.

Regenerate and test the all-features feed from the patcher service. The `-Project`
value is the workspace install id from
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`; it usually matches the
feature id, but check the registry instead of assuming it. For this checkout, the
current install id is `all-features`.

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project all-features -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project all-features -BaseUrl http://<patch-host>:8091/patcher/
```

The published feed is `http://<patch-host>:8091/patcher/all-features/`. External
testers place the generated `eqemupatcher.exe` in their EQ client root and run it.
For real external syncs, missing manifest files are release blockers. Use
`-AllowMissingClientFiles` only for partial local testing.

## Public Tester Feedback Form

The combined player feedback form pipeline lives under `features/all-features/`:

- `features/all-features/GOOGLE_FORM_PIPELINE.md`
- `features/all-features/TESTER_GOOGLE_FORM.md`
- `features/all-features/google-forms/all-features-public-test-form.gs`

Use this all-features form for public bundle testing instead of the narrower
Live Items-only form. It asks patch/login once, then covers the player-visible
tests for Live Items, Augs-in-Augs, AutoLoot, Achievements, Multiclass, Live
Spells, AI NPC Response, Dynamic Quests, Gearscore, Item Rarity, Native
Interface, HP Fix, Tradeskills, and general stability without duplicating setup
questions in each feature section.

## Verification

Use:

```powershell
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe' client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32 /m
```

Run scoped `git diff --check` on changed first-party docs/server files. The native runtime includes inherited vendor/MQ/DX scaffolding with existing whitespace, so a full-tree check is noisy until that client code is cleaned up.
