# Native Client Runtime

This folder contains the current client-side native runtime for the custom EQEmu feature set.

It supports the native EQ UI windows and transport lines for:

- Advanced Loot: `ADVLOOT|...`
- Live Items / Item Forge: `LIVEITEM|...`
- Live Spells / Spell Forge: `LIVESPELL|...`
- Achievements: `ACH|...`
- Multiclass: `MULTICLASS|...`
- GearScore / ItemPower: `ITEMPOWER|...`
- Item Rarity: `ITEMRARITY|...`
- HP Fix: `HPFIX|...`
- Faction Reputation: `FACTION|...`
- DPS Parser: `DPS|...`

## Install

The all-features patcher is the normal install path. For manual local testing, copy
the prebuilt DLL to the root of the EverQuest client folder:

```text
client_files/native_autoloot/eq-core-dll/bin/dinput8.dll
```

Copy the native UI XML files to the client's default UI folder:

```text
client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml
client_files/native_autoloot/ui/EQUI_NativeItemForgeWnd.xml
client_files/native_autoloot/ui/EQUI_NativeSpellForgeWnd.xml
client_files/native_autoloot/ui/EQUI_NativeAchievementWnd.xml
client_files/native_autoloot/ui/EQUI_NativeMulticlassWnd.xml
client_files/native_autoloot/ui/EQUI_NativeTradeskillsWnd.xml
client_files/native_autoloot/ui/EQUI_NativeHpFixWnd.xml
client_files/native_autoloot/ui/EQUI_NativeAugsInAugsWnd.xml
client_files/native_autoloot/ui/EQUI_NativeDynamicQuestsWnd.xml
client_files/native_autoloot/ui/EQUI_NativeFactionWnd.xml
client_files/native_autoloot/ui/EQUI_NativeDpsWnd.xml
```

Destination example:

```text
EverQuest/dinput8.dll
EverQuest/uifiles/default/EQUI_NativeAutoLootWnd.xml
EverQuest/uifiles/default/EQUI_NativeItemForgeWnd.xml
EverQuest/uifiles/default/EQUI_NativeSpellForgeWnd.xml
EverQuest/uifiles/default/EQUI_NativeAchievementWnd.xml
EverQuest/uifiles/default/EQUI_NativeMulticlassWnd.xml
EverQuest/uifiles/default/EQUI_NativeTradeskillsWnd.xml
EverQuest/uifiles/default/EQUI_NativeHpFixWnd.xml
EverQuest/uifiles/default/EQUI_NativeAugsInAugsWnd.xml
EverQuest/uifiles/default/EQUI_NativeDynamicQuestsWnd.xml
EverQuest/uifiles/default/EQUI_NativeFactionWnd.xml
EverQuest/uifiles/default/EQUI_NativeDpsWnd.xml
```

Only load one patched client folder at a time.

## In Game

Useful reopen commands:

```text
#advloot native show
#itemforge dialog
#livespell dialog
#ach window
#mc open
#hpfix window
#rep
#dps
```

The server remains authoritative. The DLL creates native windows, parses server
transport lines, consumes server-owned side-channel packets, and sends normal server
commands back to the zone.

## Current Shape

This is a monolithic native runtime. The feature-specific code currently lives mostly in:

```text
client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h
client_files/native_autoloot/eq-core-dll/src/native_interface.cpp
```

The old `EQUI_AoTAutoLootWnd.xml` combined prototype window is intentionally not included. Advanced Loot uses `EQUI_NativeAutoLootWnd.xml`.

The surrounding DLL project still contains older MacroQuest-derived scaffolding because the current hooks were built on that client-side base. That is separate from the removed Lua/MQ loot UI path.

## Build

Open one of these solutions in Visual Studio:

```text
client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln
client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2019.sln
```

Build `Release|Win32`. The output is:

```text
client_files/native_autoloot/eq-core-dll/bin/dinput8.dll
```
