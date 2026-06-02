# Multiclass

Standalone EQEmu feature branch for `multiclass`.

## Current Slice

This branch owns the server-side Multiclass feature boundary, the native window
XML, and the feature-local native DLL runtime code needed by Multiclass. The
current buildable slice stores one fixed trio profile per character, includes
GMAdmin-only test/profile diagnostics, exposes a player-safe native UI bridge,
and routes the first core capability gates through the Multiclass manager:
mana, caster identity, spells, disciplines, skills, item class masks, and AA
class masks. Direct equipment moves now validate wearable item class/race
against the trio mask, so a Beastlord / Magician / Necromancer can wear an item
that allows Magician or Necromancer even if Beastlord is absent, and weapon
damage/click-item checks now use the same trio eligibility. Spell/AA cast
restriction checks that are explicitly class-based now read the player's trio
mask, while NPCs and other non-player actors keep stock single-class handling.
`/mc` open/refresh resends the stock AA table with the trio AA mask so off-base
class AA entries can appear without camping after profile changes.
Combat ability
branches such as Backstab, Frenzy, Kick, Monk attacks, and Rogue utility actions
now read trio class membership instead of only the base class. Locked trios also
auto-seed eligible non-tradeskill, non-specialization class skills to `1` on
login/zone completion, level-up, trio lock, and native refresh once the
character meets the best trio train level, so off-base tools like Backstab,
Frenzy, Meditate, and instrument skills become usable without manual GM skill
patches. Item packets, previews, adventure merchant lists, and the login
bulk-inventory snapshot also clone or expand trio-usable item class masks to the
full trio so the stock client has a better chance to show and drag the item as
usable.

Native UI is the intended first player flow. Multiple-pet ownership is
implemented as a server-owned roster for eligible pet-heavy trios. Pet summon
spell effects now allow eligible locked trios to keep summoning until the
Multiclass roster cap is reached, while non-Multiclass characters keep the stock
one-pet limit. Roster pets remain owner-valid even when they are not the stock
active pet pointer, so later summons do not orphan or replace earlier pets.
Secondary roster pets persist through zoning and logout/login using reserved
`character_pet_*` slots `10..12`; the stock current pet and suspended minion
slots remain unchanged. A small Multiclass pet-state table preserves command
state that does not live cleanly in stock pet rows, including focus, hold,
spell-hold, pet order, and guard point. Group and raid pet-affinity spell fanout now includes
secondary non-charmed roster pets wherever stock logic already allows pets.
Cross-zone and world-wide spell apply/fade helpers also include secondary
Multiclass roster pets when the stock server rule allows spells on pets.

Bard trios now have an early server-authoritative Melody surface. The separate
native Melody window lists scribed Bard songs, lets the player assign up to
four slots, stores those slots in the database, and pulses selected songs from
the server without occupying the normal spell gems. Slot changes do not reset
the server pulse timer; the new mix takes effect on the next scheduled pulse.
When Melody has active slots, Bard songs cast from normal spell gems are
treated as on-demand casts instead of starting the stock auto-repeat loop.
Control songs such as charm, fear, mez, stun, root, and lull are intentionally
blocked from Melody in this first pass until they can be tested safely.

Caster-root melee trios now have a native discipline bridge. `/disc`, `/discs`,
and `/mc disc` open `NativeMulticlassDisciplineWnd`; using a row submits through
`#mc disc use <spell_id>` so Wizard-root Rogue/Berserker-style builds avoid the
stock client class rejection and still execute disciplines server-side through
the existing Multiclass capability checks. The window mirrors the stock active
discipline gauge, ticks selected reuse timers locally from server transport
metadata, and can create `/mc disc use <spell_id>` hotkeys for learned discs.

## Test Target

- Server: `D:\EQServers\EQServer-Multiclass`
- Client: `D:\EQClients\EQClient-Multiclass`
- Database: `eqemu_multiclass`

## Admin / Native Bridge Commands

- `#multiclass status`
- `#multiclass set <class1> <class2> <class3>`
- `#multiclass diag`
- `#multiclass diag skills`
- `#multiclass diag spells`
- `#multiclass diag discs`
- `#multiclass diag aa` reports trio AA mask, visible AA definition count,
  learned profile entries, first-rank purchase candidates, grant-only entries,
  and the learned-AA packet guard.
- `#multiclass diag melody`
- `#multiclass diag bonuses`
- `#multiclass diag pets`
- `#multiclass diag items`
- `#multiclass native`
- `#multiclass help`
- `#multiclass reweave grant <count>`
- `#multiclass reweave <slot 2|3> <class>`
- `#multiclassui status`
- `#multiclassui refresh`
- `#multiclassui open`
- `#multiclassui choose <class2> <class3>`
- `#mc status`
- `#mc refresh`
- `#mc open`
- `#mc choose <class2> <class3>`
- `#mc pets`
- `#mc pet refresh`
- `#mc pet focus <entity_id>`
- `#mc pet attack all`
- `#mc pet back all`
- `#mc pet follow all`
- `#mc pet guard all`
- `#mc pet health all`
- `#mc pet taunt`
- `#mc pet hold`
- `#mc pet spellhold`
- `#mc pet dismiss`
- `#mc melody open`
- `#mc melody refresh`
- `#mc melody set <slot> <spell_id>`
- `#mc melody clear [slot]`
- `#mc disc open`
- `#mc disc refresh`
- `#mc disc use <spell_id|name>`
- `#mc reweave <slot 2|3> <class>`

Players should not need a typed command loop. `#multiclassui` exists as the
explicit bridge name, while `#mc` is the shorter player-safe alias. The native
DLL rewrites `/mc`, `/multiclass`, and `/multiclassui` into `#mc` bridge calls
so `/mc` can reopen or refresh the window in a recognizable way. It also
rewrites `/mcpets`, `/multiclasspets`, and `/petui` into the standalone pet
console bridge, and `/mcmelody`, `/multiclassmelody`, `/melodyui`, or
`/mc melody` into the Bard Melody bridge. `/disc`, `/discs`, `/discipline`,
`/discwindow`, `/combatability`, and `/mc disc` now route to the native
Multiclass discipline bridge instead of the stock client class-gated command.

For melee-base caster trios, the server now sends vitals/classmask snapshots and
the native DLL applies the server-approved presentation class and mana values to
local client presentation structures. It also keeps the stock Player Window mana
gauge visible when the presentation class is a mana class and repeatedly nudges
the stock spell-gem window visible after profile/vitals refreshes. The server
remains authoritative for actual mana values; this is a HUD presentation bridge
for characters created as pure melee.

Locked trios now receive server-side resonance bonuses every ten levels from
10 through 60. The bonuses are applied during normal client bonus calculation
and are shown in the main native Multiclass window through
`MULTICLASS|bonuses|...`. Exact identities such as Grand Menagerie, Warbound
Arcanum, Spellblade Compact, Radiant Hymn, Velvet Conspiracy, Arcane Duelist
Pact, Verdant Synod, Aegis Covenant, Dirge of the Grave, and Wildcall Pact have
named tier tracks; other combinations fall back to role-based tracks.

Rare reweaves now have server-side scaffolding. Admins can grant reweaves, and
the native bridge can spend one reweave to change slot 2 or 3 in a city or
bind-safe sanctuary. Slot 1 remains tied to the character's base identity until
a deeper client-presentation strategy is proven.

## Database

Custom migration version 1 creates the initial trio profile tables:

- `custom_multiclass_profiles`
- `custom_multiclass_profile_audit`

Custom migration version 2 upgrades the earlier active-track scaffold, if
present, into the trio profile schema.

Custom migration version 3 creates `custom_multiclass_pet_state` for pet command
metadata that supplements stock `character_pet_*` persistence.

Custom migration version 4 creates `custom_multiclass_bard_melody` for the
native Melody slot assignments.

Reference SQL lives in `features/multiclass/sql/001_multiclass_schema.sql`.

## Native Client Assets

- `client_files/native_autoloot/ui/EQUI_NativeMulticlassWnd.xml`
- `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h`
- `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll`
- `features/multiclass/patcher.yml`

The runtime client DLL is feature-owned here. Multiclass does not rely on the
shared `native-client-runtime` checkout for its DLL-side `NativeMulticlassWnd`,
`NativeMulticlassPetWnd`, `MULTICLASS|...` parser, slash-command rewrites,
spellbook level patching, or spell-gem context-menu label hook.

The native runtime now parses `MULTICLASS|...` transport lines, opens
`NativeMulticlassWnd` on profile snapshots, displays the trio and presentation
class, lets an unlocked profile choose second and third classes with compact
slot selectors, renders server-owned trio role/resonance notes plus compact
skill-unlock and resonance-bonus summaries, and patches the local spellbook and spell-gem right-click
menu required-level display for scribed off-class trio spells using spell-id and
normalized-name lookup keys. It also consumes server-authored
`MULTICLASS|vitals|...` rows, keeps the stock Player Window mana gauge visible
for mana-class presentations, and retries showing the stock spell-gem window for
caster-capable trios. It hooks the client UI reset path, destroys Multiclass
runtime windows, and clears transient transport state when the client leaves the
in-game session so stale native windows do not remain over character select or
survive into the next login. Pet control is split into a separate
`NativeMulticlassPetWnd`, opened from the main window or `/mcpets`, with up to
three compact pet rows. The pet console stays intentionally slim: pet name,
mode, HP/MP, and taunt/hold/no-cast flags, with no stale target column. Its
refresh, pet focus, broadcast pet commands, and focused pet toggles submit
through the `#mc` bridge alias. Native pet buttons target the selected roster
row directly and bridge through stock EQEmu pet command handling where possible
so normal pet response text and pet-button state updates still happen. The pet
console is resizable and is released with the other Multiclass native windows
during client UI reset/logout.

Bulk spell scribing stages the server spellbook update first, sends the native
Multiclass level patch rows, then sends the client scribe packets. This keeps
the stock spell-gem right-click menus from caching off-class spells as level 255
until the player zones.

`NativeMulticlassMelodyWnd` is a separate Bard-song surface. It consumes
`MULTICLASS|melody...` transport rows, lists four server-owned slots, displays
eligible scribed songs, and submits slot changes back through `#mc melody`.

`NativeMulticlassDisciplineWnd` is a separate discipline surface. It consumes
`MULTICLASS|discipline...` transport rows, lists learned disciplines with level,
timer, total reuse duration, and readiness state, and submits use requests back
through `#mc disc`. Its child controls use SIDL `AutoStretch` anchors so the
inner list, stock active-effect gauge, footer, and buttons follow the sizable
window frame.

The feature-local client export utility emits `SkillCaps.txt` with the highest
known skill cap for each skill/level across player classes, following the useful
THJ client-file pattern while keeping real per-trio skill enforcement and
class-skill behavior on the server.

External/test-client patch syncing is driven by
`features/multiclass/patcher.yml`. Add any client-facing file there with a
repo-relative `source` and the destination path inside the EverQuest client
folder. Do not add files directly to the local EQ client as the source of truth.
The current feed includes the feature-owned `dinput8.dll`, native Multiclass
XML, generated `eqhost`, generated `EQUI.xml`, and the native XML include.
Missing files block real external syncs; `-AllowMissingClientFiles` is only for
partial local testing. Patcher deployment `-Project` values come from
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json` install ids; for this
checkout it is currently `multiclass`, but future projects should verify rather
than assuming the feature id.

`features/multiclass/TRIO_RESONANCE.md` records the current exact trio names,
fallback archetypes, and level 10 through 60 bonus tiers.

## Design Guardrails

- Keep class swaps server-authoritative.
- Prefer one fixed trio. If class changes exist later, make them rare one-slot
  reweaves earned through content.
- Current reweave scaffolding only changes slot 2 or 3 in cities or bind-safe
  sanctuaries. It is intended for future native UI/token flow, not free swapping.
- Allow multiple pet builds; pet-heavy trios are part of the intended fantasy.
- Use the native Multiclass pet console as the primary pet control surface.
  The stock pet window should remain compatibility-only, not the required way
  to focus or command individual Multiclass pets.
- The current roster supports active in-zone pets. Saving/loading all three pet
  states across zone/camp now uses reserved stock pet slots for the secondary
  roster pets plus the Multiclass pet-state table for command metadata. Death
  still follows stock active-pet clearing behavior.
- Keep Bard Melody in its own native window. The normal spell bar should stay
  free for the player's other two classes.
- Store future native UI decisions and audit rows in database-backed tables
  before enabling trio power.
- Do not implement command-only class hacks that bypass equipment, spell, skill,
  or zone restrictions.
- Keep shared identity systems character-wide unless a later design explicitly
  splits them.
- Current resonance passives are a first tunable power pass. Keep future
  balance changes centralized in `MulticlassManager::ApplyTrioBonuses`.
