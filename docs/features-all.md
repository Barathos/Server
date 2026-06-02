# All Features Bundle

This branch combines the standalone feature branches on top of a clean EQEmu base:

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

The bundle keeps the feature commits layered instead of using the dirty integration lab as source of truth. This gives operators one branch to grab when they want every system, while preserving the individual feature branches for piecemeal installs.

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
`ITEMPOWER|`, and `ITEMRARITY|` transport lines. It is no longer only lab code,
but it is still monolithic internally: most feature-specific client behavior
lives in `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h`.
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

## Verification

Use:

```powershell
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe' client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32 /m
```

Run scoped `git diff --check` on changed first-party docs/server files. The native runtime includes inherited vendor/MQ/DX scaffolding with existing whitespace, so a full-tree check is noisy until that client code is cleaned up.
