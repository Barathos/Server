# All Features Integration Audit

Last updated: 2026-06-08

## Scope

This checkout is the bundle project. Standalone feature projects were inspected and
left in their current state. Feature payloads were copied into this checkout so the
all-features package can own its client/server artifacts directly.

## Standalone Verification Snapshot

Fast workspace verification with `verify-feature.ps1 <id> -SkipBuild` found:

- Clean required-file pass: `live-items`.
- Required-file pass with dirty standalone worktree: `autoloot`, `live-spells`,
  `multiclass`, `item-rarity`, `mq-interface`, `eqemu-general-code`,
  `augs-in-augs`, `dynamic-quests`, `gearscore`.
- Standalone blockers: `achievements` and `hpfix` paths are not git checkouts.
- Required-file blockers in standalone source: `tradeskills` and
  `ai-npc-response` are missing their feature-local native DLL project files.

## Bundle Integration Completed

- Added feature payload/docs/manifests for all registered feature projects.
- Added all custom native XML windows to `client_files/native_autoloot/ui`.
- Updated `features/all-features/patcher.yml` to publish the bundle-owned DLL and
  every custom XML window.
- Added server command/build wiring for Multiclass, Item Rarity, Gearscore, and
  HP Fix command surfaces.
- Added HP Fix server-side native payload plumbing.
- Verified all files listed by `features/all-features/patcher.yml` exist.

## Build Verification

Passed in this checkout:

```powershell
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe' client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32 /m
git diff --check
```

## Remaining Integration Work

- Native DLL behavior has moved beyond the original merge checkpoint. The bundle
  DLL now handles `ADVLOOT|`, `LIVEITEM|`, `LIVESPELL|`, `ACH|`, `MULTICLASS|`,
  `HPFIX|`, `ITEMPOWER|`, `ITEMRARITY|`, `FACTION|`, and `DPS|` transport lines,
  plus native helper paths such as `/useitem`, improved autofollow, map/native
  diagnostics, and `OP_ServerAuthStats` label data.
- The custom database manifest is sequenced through custom version `19`.
  Remaining database work is feature hardening, repair/idempotency checks, and
  seed/testbed coverage rather than basic version ownership.
- Item Rarity standalone still documents a separate `client_files/item_rarity`
  DLL in places, but the all-features bundle uses one bundle-owned `dinput8.dll`;
  do not copy that separate DLL as source of truth.
- Several standalone projects remain dirty/uncommitted. This bundle checkpoint
  owns the copied payloads here, but does not make those standalone branches clean.
