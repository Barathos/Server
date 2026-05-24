# Feature Packs

This folder is the portability layer for custom systems that another EQEmu server operator might want to grab one at a time.

These are not runtime plugins. Each feature pack documents the source files, database objects, client assets, and hook points needed to lift one system into another source checkout with the smallest practical blast radius.

## Status Labels

- `draft`: boundary has been mapped, but the pack is not a clean generated patch yet.
- `portable`: has a tested patch or install sequence that can be applied to a fresh compatible checkout.
- `shared-runtime`: includes code that is still bundled with another feature or shared client runtime.
- `proved-build`: the feature has built on a clean EQEmu baseline branch.

## Current Packs

| Feature | Status | Notes |
| --- | --- | --- |
| `achievements` | `draft`, `shared-runtime`, `proved-build` | Custom achievement schema, native achievement window, player commands, and gameplay progress hooks. Native DLL code is still shared runtime. |

Use `TEMPLATE.md` when starting the next pack.
