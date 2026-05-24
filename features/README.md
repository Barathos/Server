# Feature Packs

This folder is the portability layer for custom systems that another EQEmu server operator might want to grab one at a time or as a combined bundle.

These are not runtime plugins. Each feature pack documents the source files, database objects, client assets, and hook points needed to lift one system into another source checkout with the smallest practical blast radius.

## Status Labels

- `draft`: boundary has been mapped, but the pack is not a clean generated patch yet.
- `portable`: has a tested patch or install sequence that can be applied to a fresh compatible checkout.
- `shared-runtime`: includes code that is still bundled with another feature or shared client runtime.
- `proved-build`: the feature has built on a clean EQEmu baseline branch.

## Current Packs

| Feature | Status | Notes |
| --- | --- | --- |
| `autoloot` | `draft`, `shared-runtime`, `proved-build` | Server-side AutoLoot with the native EQ AutoLoot window. Client DLL code is still partly bundled with other native windows. |
| `live-items` | `draft`, `shared-runtime`, `proved-build` | Live DB item loading, dynamic item data, item editing, and Item Forge input. Native DLL code is still shared runtime. |
| `live-spells` | `draft`, `shared-runtime`, `proved-build` | Generated spell definitions, Spell Forge input, live spell patch/sync transport, and generated scroll items. Native DLL code is still shared runtime. |

## Packaging Rules

Each feature pack should:

1. Name what the feature owns.
2. Name what it explicitly does not require.
3. List every source file that is added.
4. List every existing source file that must be patched.
5. Include feature-owned SQL as standalone files.
6. Call out any shared plumbing that still needs extraction.
7. Keep server authority on the server, especially for loot, item creation, spell mutation, achievement validation, progression, and permissions.

The goal is not to hide that source integration is needed. The goal is to make the integration honest, bounded, and repeatable.

Use `TEMPLATE.md` when starting the next pack.
