# Feature Packs

This folder is the portability layer for the standalone Achievements system that another EQEmu server operator might want to lift into a source checkout.

These are not runtime plugins. Each feature pack documents the source files, database objects, client assets, and hook points needed to lift one system into another source checkout with the smallest practical blast radius.

## Status Labels

- `draft`: boundary has been mapped, but the pack is not a clean generated patch yet.
- `portable`: has a tested patch or install sequence that can be applied to a fresh compatible checkout.
- `proved-build`: the feature has built on a clean EQEmu baseline branch.

## Current Packs

| Feature | Status | Notes |
| --- | --- | --- |
| `achievements` | `draft`, `proved-build` | Custom achievement schema, native achievement window, player commands, gameplay progress hooks, and achievements-only native DLL payload. |

Keep this folder focused on Achievements-only artifacts.
