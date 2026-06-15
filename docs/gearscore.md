# Gearscore

Design and operator documentation for the standalone `gearscore` feature.

## Overview

Gearscore adds deterministic item power scoring to the standalone `gearscore`
feature checkout. It computes raw role-power scores, derives an item level,
stores operator-auditable template scores, and exposes a progression-weighted
Gearscore for player sorting.

The first display path is the normal item display window, not a custom
Gearscore-only window. When the server sends an item packet, it also emits a
hidden `ITEMPOWER|set|...` transport line through chat transport. The
Gearscore-local native DLL consumes that payload and appends item level details
directly in the existing item display flow.

## Server Behavior

- `common/item_power.*` calculates V2 item power from `EQ::ItemData` templates
  and live `EQ::ItemInstance` values.
- Role scores are generated for tank, melee, caster, healer, and hybrid and
  remain raw role-power values for best-role selection and operator explain
  output.
- The public `item_level` is derived from the best raw role score, normalized
  by slot budget, and clamped against required/recommended level metadata.
- The displayed Gearscore is `item_level * 100 + tier_power`, where
  `tier_power` is the best raw role score normalized by slot budget and clamped
  to `0-99`.
- Live item display includes current inserted augment stats immediately by
  scoring the `EQ::ItemInstance`; augmented, dynamic, and scaling items are
  sent transiently as `source=instance`.
- Manual overrides can set item level, apply a score multiplier, apply a flat
  score bonus, or attach operator notes.
- `Client::SendItemPacket` sends hidden transport for item packets so ordinary
  inventory, merchant, and link displays can prime the native client cache.
- Missing or stale-version static `item_power` rows are calculated and saved on
  demand before transport is sent.

## Database

The feature uses compiled custom database update version `1`:

- `item_power`: stored item level, intrinsic score, role scores, score version,
  source, and update timestamp.
- `item_power_override`: optional manual level/multiplier/flat bonus/notes.
- `item_power_breakdown`: component score details for inspected/recalculated
  items.
- `item_power_search`: operator view joining current-version `item_power`,
  `items`, and optional `item_rarity` metadata for SQL/dev-tool searches.

Augmented instance scores are not persisted as separate `item_power` rows for
each augment combination. The persisted row remains the static template score;
the live ItemDisplay path recalculates the current instance when augments,
dynamic item data, or scaling data are present.

Run `.\run-db-updates.ps1 gearscore` after a successful build. Operators can
also run `#itemscore init` to create the tables during local testing.

### Operator Search View

`item_power_search` is the main server-dev surface for loot tooling and ad-hoc
SQL. It exposes `item_id`, `name`, `item_level`, `item_score`, `best_role`, all
role scores, `score_version`, `source`, item metadata, and rarity metadata.

~~~sql
SELECT *
FROM item_power_search
WHERE item_score BETWEEN 1 AND 100
  AND item_level BETWEEN 1 AND 10;
~~~

Rarity filters do not change ItemPower scoring. `item_rarity` is query metadata
that can be combined with score/level bands, not a multiplier in the scoring
model.

Search helpers only read stored current-version rows. If results are missing or
stale, run `#itemscore recalc all` or database updates before using the view for
loot generation.

## Commands

- `#itemscore init`: create/verify schema.
- `#itemscore recalc <item_id>`: calculate and store one item with breakdown.
- `#itemscore recalc all`: calculate and store all shared-memory items.
- `#itemscore show <item_id>`: show the stored score and emit the transport.
- `#itemscore explain <item_id>`: show computed role/component details.
- `#itemscore audit [limit]`: show missing rows, stale score versions, and the
  largest level deltas.
- `#itemscore search [score <min-max>] [level <min-max>] [role <role>] [rarity <rarity>] [class <alias>] [slot <alias>] [limit <1-100>]`: validate the same helper used by quest APIs.
- `#itemscore override <item_id> level <1-127>`
- `#itemscore override <item_id> multiplier <value>`
- `#itemscore override <item_id> bonus <points>`
- `#itemscore override <item_id> notes <text>`
- `#itemscore clearoverride <item_id>`
- `#itemscore view <item_id>`: request a normal item view packet to exercise
  the ItemDisplay transport path.

## Quest APIs

Perl:

~~~perl
my $power = quest::getitempower(1001);
if ($power) {
    quest::debug("Score: " . $power->{item_score});
}

my $item_id = quest::randomitempower({
    min_score => 1,
    max_score => 100,
    min_level => 1,
    max_level => 10,
    role => "melee",
    class => "rog",
    slot => "primary",
    min_rarity => "common",
});

quest::addloot($item_id) if $item_id;
~~~

Lua:

~~~lua
local power = eq.get_item_power(1001)
if power ~= nil then
    eq.debug("Score: " .. power.item_score)
end

local item_id = eq.random_item_power({
    min_score = 1,
    max_score = 100,
    min_level = 1,
    max_level = 10,
    role = "caster",
    class = "wiz,mag",
    slot = "ear",
    min_rarity = "common",
})

if item_id ~= 0 then
    e.self:AddItem(item_id, 1)
end
~~~

Perl `quest::finditempower({ ... })` returns an arrayref of hashrefs. Lua
`eq.find_item_power({ ... })` returns an array table. Result keys match the SQL
view names, including `item_id`, `item_score`, `item_level`, `best_role`,
`rarity`, and `rarity_name`.

## ItemDisplay Transport

Stored scores are serialized as:

~~~text
ITEMPOWER|set|item_id=<id>|ilvl=<level>|score=<score>|role=<role>|version=<version>|source=<source>|name=<item name>
~~~

The server emits this immediately before the item packet. If no stored
`item_power` row exists, or the stored row has an older score version, the
static template score is calculated and saved on demand before the transport is
emitted. Augmented, dynamic, and scaling items instead emit a transient
`source=instance` transport payload.

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
  ItemDisplay hook from
  `client_files/native_autoloot/eq-core-dll/src/native_interface.cpp`. No
  custom UI window is shipped; the text is appended to the normal ItemDisplay
  output only after server `ITEMPOWER|set|...` transport is cached.
- `-Project` is the workspace install id from `D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the feature id, but confirm it first.
- Patcher host commands:

~~~powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project <project-id> -BaseUrl http://<patch-host>:8091/patcher/
~~~

Missing files are release blockers for real external syncs. Use `-AllowMissingClientFiles` only for partial local testing.
