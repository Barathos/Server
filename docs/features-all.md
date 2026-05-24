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

This bundle includes the feature-owned XML windows:

- `EQUI_NativeAutoLootWnd.xml`
- `EQUI_NativeItemForgeWnd.xml`
- `EQUI_NativeSpellForgeWnd.xml`
- `EQUI_NativeAchievementWnd.xml`

The C++ native DLL runtime is still shared lab code and should be split next into a reusable `native-client-base` plus feature-specific modules. Until that split lands, this branch is the clean server/source bundle plus native XML assets.

## Verification

Use:

```powershell
cmake --preset win-msvc
cmake --build build\win-msvc --config Release --target zone -- /m
cmake --build build\win-msvc --config Release --target world -- /m
git diff --check
```
