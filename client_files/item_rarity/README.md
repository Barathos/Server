# Item Rarity Native Client

This folder contains the item-rarity-owned native client runtime.

It is separate from `EQEmu-native-client-runtime` and only handles item rarity
display support:

- parses `ITEMRARITY|...` server transport chat lines
- caches item ID to rarity tier mappings
- adds a colored rarity header to item inspect/link windows

Build `Release|Win32` from:

```text
client_files/item_rarity/eq-core-dll/item-rarity-dll.sln
```

The output is:

```text
client_files/item_rarity/eq-core-dll/bin/dinput8.dll
```

Install this DLL only to:

```text
D:\EQClients\EQClient-Item-Rarity\dinput8.dll
```
