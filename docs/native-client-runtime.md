# Native Client Runtime

This branch packages the current native client runtime needed by the custom EQEmu feature windows.

## Included Runtime

- `client_files/native_autoloot/eq-core-dll/`
- `client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml`

## Supported Server Transports

- `AUTOLOOT|...`
- `LIVEITEM|...`
- `LIVESPELL|...`
- `ACH|...`

## Not Included

The old combined prototype AutoLoot XML window is not included:

- `EQUI_AoTAutoLootWnd.xml`

AutoLoot now uses the native AutoLoot window:

- `EQUI_NativeAutoLootWnd.xml`

## Remaining Cleanup

The native runtime is still monolithic. The next cleanup step is to split `core_autoloot_native.h` into a shared native client base plus feature-specific modules.
