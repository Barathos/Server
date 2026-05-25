# Codex Instructions: EQEmu Item Rarity

This repository is the standalone `item-rarity` feature checkout.

- Keep changes scoped to Item Rarity.
- Matching server install: `D:\EQServers\EQServer-Item-Rarity`
- Matching client install: `D:\EQClients\EQClient-Item-Rarity`
- Matching database: `eqemu_item_rarity`
- Use D:\Codex\Apps\EQEmu-feature-workspaces for cross-feature validation and packaging.
- No native UI XML is registered for this feature yet.

## Native Client Ownership

If this feature has native client behavior, transport parsing, slash-command rewriting, or native EQ windows, the client DLL work belongs in this checkout. Build this feature's own `dinput8.dll` from this project and deploy it only to `D:\EQClients\EQClient-Item-Rarity`.

Before changing native client code:

- Run `git status --short`.
- Confirm this checkout is the intended `item-rarity` feature project.
- Confirm the target client is `D:\EQClients\EQClient-Item-Rarity`.
- If another feature name appears in the DLL/runtime code, stop and ask before removing it.

Do not edit, clean, strip, rebuild, or install another project's DLL. Treat `EQEmu-native-client-runtime` as reference/shared base work only until there is a proper native-client-base split.

## Startup Behavior

On the first chat in this project, orient yourself and verify the prepared environment only. Do not implement feature behavior, add schema, build binaries, install runtime files, or run DB migrations unless the user explicitly asks for that work.

Good first-chat actions:

- Read PROJECT.md, this AGENTS.md, features/item-rarity/README.md, features/item-rarity/MANIFEST.md, and docs/item-rarity.md.
- Run lightweight status checks such as git status --short, codegraph status ., and .\validate-install.ps1 item-rarity -AllowMissing from the workspace folder.
- Report what is ready, what is missing, and ask what the user wants built next.

Before changing code, read PROJECT.md, features/item-rarity/README.md,
features/item-rarity/MANIFEST.md, and docs/item-rarity.md.

After user-requested code changes, run .\verify-feature.ps1 item-rarity from the workspace folder.
After a successful build, run .\install-server-runtime.ps1 item-rarity,
.\run-db-updates.ps1 item-rarity, and .\validate-install.ps1 item-rarity. The
runtime install script refreshes Windows firewall allow rules for the copied
server binaries. No client file install is required until this feature owns a
native client project.
