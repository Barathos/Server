# Gearscore

Design and operator documentation for the standalone `gearscore` feature.

## Overview

Gearscore adds deterministic item power scoring to the standalone `gearscore`
feature checkout. It computes an intrinsic item score, derives an item level,
stores role-specific scores, and exposes audit/override tools for operators.

The first display path is the normal item display window, not a custom
Gearscore-only window. When the server sends an item packet, it also emits a
hidden `ITEMPOWER|set|...` transport line through chat transport. The
Gearscore-local native DLL consumes that payload and appends item level details
directly in the existing item display flow.

## Server Behavior

- `common/item_power.*` calculates V0 item power from `EQ::ItemData`.
- Role scores are generated for tank, melee, caster, healer, and hybrid.
- The public `item_level` is derived from the best role score, normalized by
  slot budget, and clamped against required/recommended level metadata.
- Manual overrides can set item level, apply a score multiplier, apply a flat
  score bonus, or attach operator notes.
- `Client::SendItemPacket` sends hidden transport for item packets so ordinary
  inventory, merchant, and link displays can prime the native client cache.
- Missing `item_power` rows are calculated and saved on demand before transport
  is sent.

## Database

The feature uses compiled custom database update version `1`:

- `item_power`: stored item level, intrinsic score, role scores, score version,
  source, and update timestamp.
- `item_power_override`: optional manual level/multiplier/flat bonus/notes.
- `item_power_breakdown`: component score details for inspected/recalculated
  items.

Run `.\run-db-updates.ps1 gearscore` after a successful build. Operators can
also run `#itemscore init` to create the tables during local testing.

## Commands

- `#itemscore init`: create/verify schema.
- `#itemscore recalc <item_id>`: calculate and store one item with breakdown.
- `#itemscore recalc all`: calculate and store all shared-memory items.
- `#itemscore show <item_id>`: show the stored score and emit the transport.
- `#itemscore explain <item_id>`: show computed role/component details.
- `#itemscore audit [limit]`: show missing rows, stale score versions, and the
  largest level deltas.
- `#itemscore override <item_id> level <1-127>`
- `#itemscore override <item_id> multiplier <value>`
- `#itemscore override <item_id> bonus <points>`
- `#itemscore override <item_id> notes <text>`
- `#itemscore clearoverride <item_id>`
- `#itemscore view <item_id>`: request a normal item view packet to exercise
  the ItemDisplay transport path.

## ItemDisplay Transport

Stored scores are serialized as:

~~~text
ITEMPOWER|set|item_id=<id>|ilvl=<level>|score=<score>|role=<role>|version=<version>|source=<source>|name=<item name>
~~~

The server emits this immediately before the item packet. If no stored
`item_power` row exists, the score is calculated and saved on demand before the
transport is emitted.

## Local Verification

- Build: `.\verify-feature.ps1 gearscore`
- Initialize runtime baseline: `.\initialize-server-runtime-template.ps1 gearscore -Force -GrantRuntimeDbUser`
- Install runtime: `.\install-server-runtime.ps1 gearscore`
- Install client files: `.\install-client-files.ps1 gearscore`
- Run DB updates: `.\run-db-updates.ps1 gearscore`
- Validate install: `.\validate-install.ps1 gearscore`

## External Client Sync

- Client patch manifest: `features/gearscore/patcher.yml`
- Client files include `dinput8.dll` so the Gearscore client can load the
  ItemDisplay hook locally. No custom UI window is shipped; the text is
  appended to the normal ItemDisplay output.
- `-Project` is the workspace install id from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but confirm it first.
- Patcher host commands:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

Missing files are release blockers for real external syncs. Use `-AllowMissingClientFiles` only for partial local testing.
