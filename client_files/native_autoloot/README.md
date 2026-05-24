# Native AutoLoot Client Files

This folder contains the AutoLoot-specific native UI XML for the matching server branch.

The DLL source is not included in this proof branch yet. The current DLL implementation still lives in shared native-client runtime code and needs a `native-client-base` split before the client side is a clean AutoLoot-only pack.

## Install

Copy the SIDL XML file to the client's default UI folder:

```text
client_files/native_autoloot/ui/EQUI_NativeAutoLootWnd.xml
```

Destination example:

```text
EverQuest/uifiles/default/EQUI_NativeAutoLootWnd.xml
```

Only load one patched client folder at a time.

## In Game

Use this command to reopen the window if it is closed:

```text
#autoloot native show
```

The server remains authoritative. The native client DLL host only creates the native window, parses `AUTOLOOT|...` status lines, and sends normal server commands back to the zone.
