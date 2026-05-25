# First Codex Chat Message

Paste this into a new Codex chat opened in this checkout:

~~~text
We are in the EQEmu Item Rarity feature project.

Please start in orientation mode only.

Read PROJECT.md, AGENTS.md, features/item-rarity/README.md, features/item-rarity/MANIFEST.md, and docs/item-rarity.md. Then do only lightweight environment checks:
- git status --short
- codegraph status .
- from D:\Codex\Apps\EQEmu-feature-workspaces, run .\validate-install.ps1 item-rarity -AllowMissing

Do not implement feature behavior, add schema, build binaries, install runtime files, run DB migrations, or edit files yet. After the checks, summarize what is ready/missing and ask me what I want to build next.

This project should stay isolated as the standalone Item Rarity feature branch. Prepared local test environment:
- Source: D:\Codex\Apps\EQEmu-feature-item-rarity
- Server: D:\EQServers\EQServer-Item-Rarity
- Client: D:\EQClients\EQClient-Item-Rarity
- Database: eqemu_item_rarity
- Login client port: 5999
- Native client DLLs, when present, are feature-owned and must be built from this checkout, not copied from EQEmu-native-client-runtime.
~~~