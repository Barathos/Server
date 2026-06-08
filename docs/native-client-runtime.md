# Native Client Runtime

This branch packages the current native client runtime needed by the custom EQEmu feature windows.

## Included Runtime

- `client_files/native_autoloot/eq-core-dll/`
- `client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeMulticlassWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeTradeskillsWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeHpFixWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeAugsInAugsWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeDynamicQuestsWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeFactionWnd.xml`
- `client_files/native_autoloot/ui/EQUI_NativeDpsWnd.xml`

## Supported Server Transports

- `ADVLOOT|...`
- `LIVEITEM|...`
- `LIVESPELL|...`
- `ACH|...`
- `MULTICLASS|...`
- `ITEMPOWER|...`
- `ITEMRARITY|...`
- `HPFIX|...`
- `FACTION|...`
- `DPS|...`

The runtime also consumes the custom `OP_ServerAuthStats` packet for server-owned
label data and includes native helper behavior for `/useitem`, improved autofollow,
map/native interface diagnostics, and ItemDisplay decoration.

## Not Included

The old combined prototype AutoLoot XML window is not included:

- `EQUI_AoTAutoLootWnd.xml`

Advanced Loot now uses the native Advanced Loot window:

- `EQUI_NativeAutoLootWnd.xml`

AI NPC Response, Pet Bags, UseItem, Autoskills, Server Auth Stats, Improved
AutoFollow, and Fellowships do not currently publish separate custom XML windows in
this bundle. They are server-command, stock-UI, native-helper, or side-channel
systems.

## Remaining Cleanup

The native runtime is still monolithic. The next cleanup step is to split `core_autoloot_native.h` into a shared native client base plus feature-specific modules.
