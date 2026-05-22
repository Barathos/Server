# Native AutoLoot Client Files

This folder contains the client-side files needed to test the native AutoLoot prototype with the matching server branch.

## Install

Copy the prebuilt DLL to the root of the EverQuest client folder:

```text
client_files/native_autoloot/eq-core-dll/bin/dinput8.dll
```

Copy the SIDL XML file to the client's default UI folder:

```text
client_files/native_autoloot/ui/EQUI_AoTAutoLootWnd.xml
```

Destination example:

```text
EverQuest/dinput8.dll
EverQuest/uifiles/default/EQUI_AoTAutoLootWnd.xml
```

Only load one patched client folder at a time.

## In Game

Use this command to reopen the window if it is closed:

```text
#autoloot native show
```

The server remains authoritative. The DLL only creates the native window, parses `AUTOLOOT|...` status lines, and sends normal server commands back to the zone.
