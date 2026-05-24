# All Features Bundle

This branch combines the standalone feature branches on top of a clean EQEmu base:

- AutoLoot
- Live Items / Item Forge
- Live Spells / Spell Forge
- Achievements

## Branch Shape

The bundle keeps the feature commits layered instead of using the dirty integration lab as source of truth. This gives operators one branch to grab when they want every system, while preserving the individual feature branches for piecemeal installs.

## Database

The combined custom database manifest uses:

- Custom version `1`: AutoLoot schema.
- Custom version `2`: Achievement schema.
- Custom version `3`: Achievement catalog seed.
- Custom version `4`: Live hunter achievement seed.

`common/version.h` sets `CUSTOM_BINARY_DATABASE_VERSION` to `4`.

## Native Client Assets

This bundle includes the native DLL runtime plus the feature-owned XML windows:

- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`
- `client_files/native_autoloot/eq-core-dll/`

- `EQUI_NativeAutoLootWnd.xml`
- `EQUI_NativeItemForgeWnd.xml`
- `EQUI_NativeSpellForgeWnd.xml`
- `EQUI_NativeAchievementWnd.xml`

The runtime handles `AUTOLOOT|`, `LIVEITEM|`, `LIVESPELL|`, and `ACH|` transport lines. It is no longer only lab code, but it is still monolithic internally: most feature-specific client behavior lives in `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h`. The next cleanup is splitting that into a reusable native-client base plus feature-specific native modules.

## Verification

Use:

```powershell
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe' client_files\native_autoloot\eq-core-dll\eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32 /m
```

Run scoped `git diff --check` on changed first-party docs/server files. The native runtime includes inherited vendor/MQ/DX scaffolding with existing whitespace, so a full-tree check is noisy until that client code is cleaned up.
