# Achievements Feature Pack

Status: `draft`, `feature-owned-runtime`, `proved-build`

This pack describes the custom Achievement system: database-backed achievement categories/objectives/progress, player-facing commands, gameplay progress hooks, and the native Achievement window transport.

## What This Feature Owns

- Custom achievement database schema and seed catalog.
- Character achievement progress and completion tracking.
- Achievement reward definitions, claim queues, and Live Item reward requests.
- Level, zone visit, task completion, skill, and kill objective processing.
- `#ach`, `#achievement`, and `#achievements` command aliases.
- Native Achievement window transport using `ACH|...` lines.

## What This Feature Does Not Require

- AutoLoot.
- Live Items or Item Forge.
- Live Spells or Spell Forge.
- MacroQuest, Lua, ImGui, or overlay UI code.

## Dependencies

- EQEmu source rebuild.
- Custom database migrations in `common/database/database_update_manifest_custom.h`.
- `CUSTOM_BINARY_DATABASE_VERSION 4` in `common/version.h`.
- This feature checkout's own native client DLL host if the operator wants the in-client Achievement window.

## Install Outline

1. Apply the source files and hook patches listed in `MANIFEST.md`.
2. Copy the custom migration entries or run the branch's `database_update` flow.
3. Rebuild `zone` and `world`.
4. Deploy `EQUI_NativeAchievementWnd.xml` and include it from the target client's `EQUI.xml`.
5. Log in and run `#ach window` or `#ach status`.
6. Run `#ach rewards` and `#ach claim all` after completing a rewarded achievement.

## Smoke Test

1. Run `#ach status` on a character.
2. Run `#ach categories` and `#ach category 1`.
3. Zone once and confirm zone visit objectives can update.
4. Complete a task or use a known seeded objective trigger.
5. Open `#ach window` and confirm the native client receives `ACH|...` lines.
6. Complete level or tradeskill milestones and confirm title/coin rewards auto-claim while Live Item requests enter `custom_achievement_live_item_requests`.

## Native Runtime Boundary

This branch includes the native Achievement XML. Any C++ DLL implementation that listens for `ACH|...` must live in this feature checkout and deploy only to the matching Achievements client folder.
