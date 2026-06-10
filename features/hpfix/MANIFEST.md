# HP Fix Manifest

Feature id: `hpfix`

This feature keeps the normal RoF2 HP packet path intact and adds an opt-in
native side-channel for corrected self HP display above the client-side 10M-ish
display failure point.

## Test Target

- Server: `D:\EQServers\EQServer-Hp-Fix`
- Client: `D:\EQClients\EQClient-Hp-Fix`
- Database: `eqemu_hpfix`

## Feature Files

- `zone/gm_commands/hpfix.cpp`: hidden native handshake, forced refresh, and GM test item summons.
- `zone/client.cpp`, `zone/client.h`, `zone/mob.cpp`: native-ready state and self HP side-channel dispatch.
- `client_files/native_autoloot/eq-core-dll`: feature-owned DLL project for the client hook and overlay.
- `client_files/native_autoloot/ui/EQUI_NativeHpFixWnd.xml`: corrected HP overlay window.
- `features/hpfix/sql/001_hpfix_test_items.sql`: high-HP validation equipment.
- `features/hpfix/sql/002_hpfix_live_test_items.sql`: live-range high-HP validation equipment.
