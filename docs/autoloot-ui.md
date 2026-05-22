# AutoLoot Native UI Integration

## Current Path

AutoLoot's primary UI in this build is the native EQ client window created by the `dinput8.dll` prototype:

`D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\eq-core-dll\src\core_autoloot_native.h`

The SIDL layout lives here:

`D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\ui\EQUI_AoTAutoLootWnd.xml`

The server remains authoritative for every item transfer, rule change, and group decision. The native client window only displays server state and sends server commands.

## What Is Wired Today

- Main `Advanced Loot`-style window with personal loot and shared loot lists.
- Separate `AutoLoot Filters` window for saved Keep/Never rules.
- Server snapshots use `AUTOLOOT|...` chat protocol lines that the native DLL parses.
- The native DLL sends `#autoloot native status`, `#lootfilter native list both`, and action commands back through chat.
- Player-facing rules are `Keep`, `Never`, and `Unset`; the DB still stores `include` and `exclude` internally for compatibility.
- Stackables use the source inventory path directly; there is no stack buffer.

## In-Game Commands

Use these from the EQ client when the native window is loaded:

```text
#autoloot native show
#autoloot native status
#lootfilter native list both
```

Regular server commands still own behavior:

```text
#lootfilter keep <item_id>
#lootfilter ignore <item_id>
#lootfilter unset <item_id>
#autoloot action <entry_id> loot
#autoloot action <entry_id> leave
```

## Wire Model

The server emits compact protocol lines:

```text
AUTOLOOT|window|show
AUTOLOOT|status|enabled=...|include=...|exclude=...|grouped=...|group_mode=...|assigned=...|leader=...
AUTOLOOT|snapshot|begin
AUTOLOOT|entry|scope=personal|id=...|item_id=...|icon=...|name=...|qty=...|source=...|state=...|rule=...
AUTOLOOT|filter|mode=include|item_id=...|icon=...|name=...
AUTOLOOT|snapshot|end
```

The DLL parses those lines, refreshes the native windows, and hides the transport spam from the player.

## Rules UI

The rules window is separate because Keep/Never lists can grow large. It supports:

- item name and item ID columns
- current rule color
- selected-row Keep, Never, and Unset buttons
- immediate refresh after rule changes
- SIDL `AutoStretch` anchors so the list resizes with the window

Rule meaning:

- `Keep`: auto-loot this item.
- `Never`: leave this item on the corpse.
- `Unset`: no saved rule; show the item as a pending loot decision.

## Build Notes

Build the native DLL from:

`D:\Codex\Apps\Everquest-autoloot-native-ui\client_files\native_autoloot\eq-core-dll`

Deploy outputs to the isolated test client:

```text
D:\Codex\Apps\Everquest-EQ-autoloot-native-ui\dinput8.dll
D:\Codex\Apps\Everquest-EQ-autoloot-native-ui\uifiles\default\EQUI_AoTAutoLootWnd.xml
```

Build and deploy `zone.exe` from the isolated source tree to:

```text
D:\server-autoloot-native-ui\bin
```
