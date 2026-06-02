# Multiclass

Design and operator documentation for the standalone `multiclass` feature.

## Overview

Multiclass is intended to let one character identity play as one fixed trio of
classes at the same time, closer to The Heroes' Journey than to an active-class
swap system. The trio should feel like an in-world identity selected through
the native client UI, not a typed command loop.

Current behavior:

- The server stores one trio profile per character in
  `custom_multiclass_profiles`.
- `#multiclass status` is an admin/native bridge command that reports the
  target's trio profile and seeds slot 1 from the current class if no profile
  exists.
- `#multiclass set <class1> <class2> <class3>` is a GMAdmin-only test bridge
  that writes a locked trio profile for the target client.
- `#multiclass diag` reports the target's trio masks, caster flags, and sample
  skill caps for development validation.
- `#multiclass diag skills`, `#multiclass diag spells`,
  `#multiclass diag discs`, `#multiclass diag aa`, and
  `#multiclass diag melody` provide focused
  development diagnostics for the main remaining proof surfaces.
- `#multiclass diag bonuses`, `#multiclass diag pets`, and
  `#multiclass diag items` provide focused checks for resonance tiers, roster
  pet state, and item-mask eligibility.
- `#multiclass reweave grant <count>` grants rare one-slot reweaves to the
  target, and `#multiclass reweave <slot 2|3> <class>` is an admin test path
  for slot changes.
- `#mc reweave <slot 2|3> <class>` consumes one granted reweave through the
  native bridge. Reweaves require a locked trio, are limited to slot 2 or 3,
  and only work in cities or bind-safe sanctuaries.
- `#multiclass native` emits a `MULTICLASS|profile|...` snapshot for future
  native client UI work.
- `#multiclassui` is a Player-accessible native bridge. The DLL uses it for
  profile snapshots and server-authoritative trio locking; it is not intended
  as a typed player loop.
- `#mc` is the shorter Player-accessible alias for the native bridge, and the
  native DLL rewrites `/mc`, `/multiclass`, and `/multiclassui` into `#mc`
  bridge calls. `/mcpets`, `/multiclasspets`, and `/petui` open the standalone
  Multiclass pet console. `/mcmelody`, `/multiclassmelody`, `/melodyui`, and
  `/mc melody` open the Bard Melody window for Bard trios.
- `/disc`, `/discs`, `/discipline`, `/discwindow`, `/combatability`, and
  `/mc disc` open the native Multiclass discipline bridge. The bridge lists
  learned disciplines with level/reuse/readiness state, mirrors the stock active
  discipline timer gauge, can create `/mc disc use <spell_id>` hotkeys, and
  submits use requests through `#mc disc use <spell_id>`, bypassing stock client
  class rejection for caster-root melee trios while keeping the server
  authoritative.
- `EQUI_NativeMulticlassWnd.xml` now provides a native Multiclass window with
  profile display, compact class slot selectors for unlocked profiles,
  refresh/lock actions, server-owned trio notes, plus a separate native
  `NativeMulticlassPetWnd` pet console in the same XML package.
- The Multiclass checkout owns its native DLL runtime code and build artifact.
  Its feature-local `dinput8.dll` handles the `MULTICLASS|...` parser,
  `NativeMulticlassWnd`, `NativeMulticlassPetWnd`,
  `NativeMulticlassDisciplineWnd`, `/mc`, `/mcpets`, and `/disc` command
  rewrites, spellbook level patching, spell-gem context-menu label hook, caster
  spell-gem window visibility nudge, and native window cleanup when leaving the
  in-game session.
- Eligible pet-heavy trios can maintain up to three active in-zone pets. The
  summon spell path now honors the Multiclass roster limit before showing the
  stock one-pet denial, and the standalone native pet console shows one compact
  row per pet and routes broadcast or focused pet commands back to the server.
  Roster pets remain owner-valid even when they are not the stock active pet
  pointer, so later summons do not orphan earlier roster pets.
- Secondary Multiclass roster pets persist through zoning and logout/login.
  The focused/current pet continues to use stock `character_pet_*` slot `0`,
  suspended minion remains slot `1`, and secondary Multiclass pets use reserved
  slots `10..12`. This preserves stock pet HP, mana, buffs, worn equipment,
  size, petpower, and taunt state without changing the base pet table schema.
  `custom_multiclass_pet_state` stores the supplemental command metadata that
  stock pet rows do not cover, such as native focus, hold, spell-hold, pet
  order, stop/regroup flags, and guard point.
  Group/raid pet-affinity spell fanout, generic spell-apply lists, and
  cross-zone/world-wide spell apply/fade helpers include secondary roster pets
  whenever the stock rules already include pets.
- Bard trios have a separate native `NativeMulticlassMelodyWnd`. It lists
  scribed Bard songs, stores up to four selected songs, and pulses those songs
  server-side without occupying normal spell gems. Slot edits preserve the
  current pulse cadence, so changing one song cannot force an immediate refresh
  of the whole stack. When Melody has active slots, Bard songs cast from normal
  spell gems are treated as on-demand casts and do not start the stock
  auto-repeat loop; this leaves charm, mez, and other special-use songs
  available without creating a second hidden melody. The first pass blocks
  charm, fear, mez, stun, root, and lull songs from the Melody window until
  they can be tested safely.
- Caster-root melee trios have a separate native
  `NativeMulticlassDisciplineWnd`. It is the first discipline/combat-tool
  fallback for builds such as Wizard / Rogue / Berserker when the stock client
  refuses `/disc` because the presentation class is not a melee class.
- Locked trios receive server-side resonance bonuses every ten levels from
  10 through 60. Exact trio identities such as Grand Menagerie, Warbound
  Arcanum, Spellblade Compact, Radiant Hymn, Velvet Conspiracy, Arcane Duelist
  Pact, Verdant Synod, Aegis Covenant, Dirge of the Grave, and Wildcall Pact
  have named tier tracks and small accent bonuses; all other trios fall back to
  role-based tracks. The main native Multiclass window shows the active and
  next tier through `MULTICLASS|bonuses|...`.
- The first capability layer is live for core server gates: mana/caster
  identity, spell and discipline levels, scroll/tome learning, spell
  memorizing, fizzle math, spell-group ranking, skill caps/training, item class
  masks, direct equipment moves, guild-bank usability, loot auto-equip, class
  skill actions, click item usability, item-click restrictions, weapon damage
  eligibility, normal and bulk item-packet class-mask presentation, item preview
  presentation, adventure merchant presentation, class-based spell/AA cast
  restrictions, and AA class masks. `/mc` open/refresh resends the AA table
  with the trio AA mask so the stock AA window can rebuild off-base class
  entries after a profile change or manual refresh.
- Locked trios now auto-seed eligible non-tradeskill, non-specialization class
  skills to `1` when the trio is locked, on login/zone completion, after level
  changes, and when the native profile refreshes. This gives off-base tools
  like Backstab, Frenzy, Meditate, combat defenses, casting skills, tracking,
  forage, and instruments a real learned value without maxing them or granting
  tradeskills/specializations.
- Native snapshots include scribed-spell level patches for off-presentation
  trio spells. The DLL applies the server's best trio level to the client's
  current presentation class slot so the stock spellbook does not show usable
  Necromancer, Beastlord, or other added-class spells as level 255. The native
  runtime also rewrites cached spell-gem right-click menu labels to the same
  server-approved levels, using spell-id lookup when the client exposes a menu
  id and normalized spell-name lookup as the fallback. Bulk scribing now sends
  the native level patch before the client scribe packets so newly built
  spell-gem right-click menus do not cache level 255 until the next zone.
- Native snapshots include server-authored vitals, class masks, presentation
  class, base class, current mana, max mana, current endurance, and max
  endurance. For melee-base caster trios, the native runtime applies that data
  to local presentation structures and keeps the stock Player Window mana gauge
  and mana labels visible when the server-selected presentation class is a mana
  class. It also retries showing the stock spell-gem window after profile and
  vitals snapshots so melee-created caster trios do not need to press Alt+S
  before testing spells. The server still owns the real mana calculation; the
  DLL is a client HUD presentation bridge.
- The feature-local client export utility now emits Multiclass `SkillCaps.txt`
  data by taking the highest known skill cap for each skill/level across player
  classes. This follows the useful THJ client-file pattern while keeping actual
  per-character enforcement server-authoritative through the Multiclass
  capability layer.
- The target client's `uifiles/default/EQUI.xml` must include
  `EQUI_NativeMulticlassWnd.xml`. The feature workspace installer now handles
  this include automatically.
- External/test-client patch syncing is owned by
  `features/multiclass/patcher.yml`. Each listed file maps a repo-relative
  `source` to the path it should occupy inside the EverQuest client folder. The
  current Multiclass patch feed ships the feature-owned native `dinput8.dll`,
  `EQUI_NativeMulticlassWnd.xml`, generated `eqhost`, generated `EQUI.xml`, and
  the native XML include. Add future client-facing assets to this file before
  syncing external testers; do not add files directly to the local EQ client as
  the source of truth. Missing files are release blockers for real external
  syncs; use `-AllowMissingClientFiles` only for partial local testing.

The intended player flow is native UI first. Trainer hails are not part of the
current plan, and typed commands are temporary admin/native bridge scaffolding
only.

## Feature Shape

- A character has one trio: slot 1, slot 2, and slot 3.
- All selected classes should eventually be considered active together for
  spells, discs, skills, AA access, item eligibility, and class-defining tools.
- Multiple pets are part of the design. Profiles store
  `multiple_pets_enabled`, and pet-heavy trios should be allowed to become real
  multi-pet builds.
- Trio identity matters. Profiles store a `trio_name` and `resonance_key` for
  exact names such as Grand Menagerie, Warbound Arcanum, Spellblade Compact,
  Radiant Hymn, Velvet Conspiracy, Arcane Duelist Pact, Verdant Synod, Aegis
  Covenant, Dirge of the Grave, and Wildcall Pact.
- The server now emits derived trio metadata for the native UI: role tags,
  resonance label, short summary, pet policy, pet-control direction, and
  resonance-bonus summary.
- Trio changes should be rare and deliberate. Profiles store
  `reweaves_available` for future quest/token-based one-slot swaps.

## Database

Custom migration version 1 creates:

- `custom_multiclass_profiles`: one row per character with three class slots,
  trio naming, multiple-pet policy, lock state, and future reweave count.
- `custom_multiclass_profile_audit`: reserved audit rows for future native UI,
  unlock, reweave, and admin actions.

Custom migration version 2 upgrades the local first-slice active-track scaffold,
if it exists, into the trio profile schema and removes the old active-track
tables.

Custom migration version 3 creates:

- `custom_multiclass_pet_state`: supplemental command-state rows for persisted
  Multiclass pets, keyed by character and reserved pet slot.

Custom migration version 4 creates:

- `custom_multiclass_bard_melody`: four native Bard Melody slot assignments per
  character.

Reference SQL is kept in
`features/multiclass/sql/001_multiclass_schema.sql`, while the runtime migration
source of truth is `common/database/database_update_manifest_custom.h`.

## Remaining Non-Goals For This Slice

- No player-facing command loop.
- No trainer hail implementation yet.
- No full trainer-based class slot selection.
- No pet persistence through death; dead characters still clear active pet state
  the same way stock pet persistence does.
- No full client hotbar/window presentation strategy yet if the stock client
  hides spellbook, spell gems, or discipline controls for some base classes.
- No Bard Melody control-song automation yet; charm, fear, mez, stun, root, and
  lull songs are intentionally disabled in the melody window until separately
  validated.
- No full native reweave UX yet. The server bridge exists, but token flow,
  confirmation UI, and player-facing polish are still future work.
- No final balance pass on resonance bonuses yet. The current numbers are a
  feature-complete first pass intended for manual tuning.
- No quest-script or client-authoritative class changes.

## Future Data Model Direction

Before enabling trio power, extend the source-backed behavior for:

- native UI slot selection and profile locking
- class eligibility helpers used by spells, discs, skills, AAs, and items
- multiple-pet ownership and command policy
- trio resonance balance tuning and additional exact trio identity data
- native UI profile display and selection confirmation
- native rare one-slot reweave confirmation and token flow

Any reweave path should be server-authoritative and restricted to safe locations
such as cities, guild halls, or future class sanctums.

## Source-Dive Facts

- The EQ client and EQEmu player profile remain single-class at the identity
  layer. `PlayerProfile_Struct::class_`, `Client::GetBaseClass()`, and
  `Mob::GetClass()` are still the login/profile class contract.
- The current Multiclass profile stores the intended trio and now routes many
  spell, discipline, skill, AA, mana, item, presentation, and pet-summon checks
  through the capability layer. Eligible pet-heavy trios use an in-zone owned
  pet roster instead of repeatedly rotating `SetPet()`.
- Most server class gates are reachable through predictable systems: spell
  level arrays, item class bitmasks, skill caps, AA class masks, caster/mana
  helpers, and pet ownership.
- Some combat and class-flavor logic uses direct class checks. Those should be
  handled deliberately instead of blindly treating every selected class as the
  base class everywhere.
- No source-level blocker has been found. The main proof points are client UI
  behavior for non-caster base classes, AA packet/client limits, edge-case item
  presentation, and clean multiple-pet ownership through death/reconnect cases.

## Design Decisions

- Multiclass is a fixed trio identity, not an active-class swap system.
- Player-facing selection and management should happen through native UI. Typed
  player commands are not part of the desired flow.
- The server is authoritative. Native UI and `dinput.dll` integration should be
  display/input transport, not the source of truth.
- The base class remains the character's profile identity unless a later client
  proof requires a more advanced presentation strategy.
- Behavior should be routed through a small capability layer before broad
  feature work starts. Avoid scattering inline `HasClass()` checks through the
  codebase.
- The first proof trio should include a melee base with caster additions, such
  as Warrior / Magician / Wizard, because that stresses spellbar, spellbook,
  mana, and pet behavior early.

External benchmark notes:

- The Heroes' Journey model is the closest target: a player chooses second and
  third class paths by talking to class guild masters and is effectively all
  three classes at once, including spells, AA, discs, skills, and itemization.
- EverQuest Legends appears to be a more curated/balanced official take:
  public coverage describes early multiclass selection, loadouts, modernized UI,
  small-group/solo goals, and a more handpicked benefit set.
- This branch should skew toward the THJ feel for player power and build
  experimentation, while keeping the implementation cleaner through
  server-authoritative helpers, deliberate client UI, and a real pet roster.

## Capability Layer Direction

The Multiclass capability layer should answer questions like:

- Which classes are in this character's trio?
- What normal item class mask does the trio allow?
- What shifted AA class mask does the trio allow?
- What is the best usable spell level for a spell across the trio?
- What is the best skill cap and train level for a skill across the trio?
- Is this character an INT caster, WIS caster, hybrid caster, pure melee, pet
  class, or bard-like special case for this specific system?
- Can this item, spell, discipline, AA rank, skill, or class tool be used by the
  trio?

Initial hook coverage:

- Mana and caster identity: `Client::CalcMaxMana()`,
  `Client::CalcBaseMana()`, `Mob::IsIntelligenceCasterClass()`, and
  `Mob::IsWisdomCasterClass()`.
- Spells and spellbook: scribing, memorizing, casting checks, fizzle checks,
  spell-group ranking, spell scroll eligibility, and class-based cast
  restrictions used by spells, disciplines, and active AA effects.
- Disciplines: tome learning, discipline use, discipline auto-learning, and
  discipline packet updates.
- Items: direct equipment swaps, weapon damage eligibility, bonus application,
  click restrictions, merchant class restrictions, guild-bank usability display,
  and loot auto-equip. Outgoing item packets, item previews, adventure merchant
  lists, and the login bulk-inventory snapshot use per-client clones or
  presentation-only masks for trio-usable items without mutating shared database
  item data.
- Skills and combat tools: skill caps, trainer caps, trio trainer access,
  class-defining checks such as dual wield, dodge, parry, riposte, block,
  triple attack, tracking, forage, hide, sneak, taunt, mend, Backstab, Frenzy,
  Kick, Monk special attacks, Rogue utility actions, and bard/instrument
  behavior. Eligible non-tradeskill, non-specialization skills are seeded to
  `1` automatically once the locked trio reaches the best train level; normal
  use/training then grows those skills under the trio cap.
- AA: table sending, rank use/purchase checks, client class masks, and learned
  AA persistence. Learned-AA packet sending now guards against the fixed client
  profile array limit instead of writing past the stock storage.
- Pets: summon-spell one-pet checks, in-zone roster capacity, focused-pet
  tracking, owner resolution for non-focused roster pets, native pet-console
  snapshots, broadcast/focused command routing, secondary pet persistence, and
  group/raid plus cross-zone/world-wide pet spell fanout.
- Bard Melody: native four-song slot storage, scribed-song validation, blocked
  control-song policy, and server-side pulses independent of normal spell gems.
- Resonance: locked-trio passive bonuses, exact trio identities, fallback
  role-based tier names, and native summary transport.
- Reweave: rare slot 2 or 3 replacement after an admin grant, restricted to
  safe locations and audited through the profile save path.

The current helper lives in `zone/multiclass_manager.*` and exposes class slots,
normal item class masks, shifted AA masks, best spell level, best skill cap,
best skill train level, caster flags, bard flags, item usability, and secondary
pet-roster targets for spell fanout. New hooks should prefer these helpers over
direct inline trio checks.

## Native UI Direction

The current XML is a real native window surface backed by a feature-local
Multiclass build of `dinput8.dll`. The server emits `MULTICLASS|...` transport
lines, and the DLL opens `NativeMulticlassWnd`, refreshes profile state,
patches client spell display data, and submits locked-trio choices back through
`#mc choose`.

Current login/native flow:

- On login, the DLL requests `#mc status`; the server responds with a
  native snapshot and `MULTICLASS|window|show`.
- Players can reopen or refresh the window with `/mc`; the DLL rewrites the
  slash command into the server-owned `#mc` bridge.
- Once a profile snapshot has a mana-class presentation, the DLL re-shows the
  stock Player Window `Player_Mana`, `Player_ManaLabel`, and
  `Player_ManaPercLabel` pieces during native pulses.
- Each native profile refresh also sends hidden spellbook level patch rows for
  scribed spells whose best trio level differs from the current presentation
  class level. The DLL keeps those levels by spell id and spell name, then
  rewrites spell-gem context menu leaf labels as the client builds the
  right-click menus.
- Each native profile refresh sends `MULTICLASS|vitals|...` after the profile
  row. The DLL stores current/max mana and endurance, class masks, presentation
  class, and base class; it uses those values to patch local client presentation
  data and to show the server mana value in the Multiclass window.
- Each native profile refresh also sends `MULTICLASS|skills|...` with a compact
  server-authored skill readiness summary. The main Multiclass window folds this
  into the trio notes area without turning the pet console back into a mixed
  control surface.
- Each native profile refresh also sends `MULTICLASS|bonuses|...` with the
  current locked-trio resonance tier summary and next unlock.
- Players can open the pet console with `/mcpets`, `/multiclasspets`, `/petui`,
  or the Pets button in the Multiclass window; pet commands request
  `MULTICLASS|pet_window|show` without reopening the trio window.
- Native pet buttons target the selected roster row directly. Server-side,
  those commands temporarily make that roster pet the active command pet and
  use the stock EQEmu pet command path where possible, preserving familiar pet
  response text and client pet-button state updates.
- Bard trios can open `/mcmelody`, `/multiclassmelody`, `/melodyui`, or
  `/mc melody`. The separate Melody window lists four slots and eligible scribed
  songs, then submits `#mc melody set` or `clear` requests to the server.
- Caster-root melee trios can open `/disc`, `/discs`, `/discipline`,
  `/discwindow`, `/combatability`, or `/mc disc`. The separate discipline
  window lists learned disciplines and uses `#mc disc use <spell_id>` so the
  server path runs through `Client::UseDiscipline` and Multiclass spell-level
  checks instead of relying on the stock client to allow the command. The
  window mirrors the stock active-combat-effect gauge (`EQType 26`), shows
  server-authored reuse timers that tick down locally between refreshes, and
  creates command hotkeys for the selected discipline.
- Every native window we add needs an explicit resize contract. If the shell is
  sizable, every child control must either use SIDL `AutoStretch` anchors or be
  repositioned from that window's `Layout()` method, and the pulse loop must
  call that `Layout()` method. If a window has no resize behavior, keep
  `Style_Sizable` false.
- The first view shows the current base class, selected second and third
  classes, presentation class, lock state, multiple-pet flag, and reweaves.
- If the profile is unlocked, the player can select second and third classes
  from compact native slot selectors and lock them server-side.
- Confirmation must call back into server-authoritative selection logic. The
  client UI should never directly mutate the profile.
- Once the trio is locked, the Multiclass window becomes a profile/trio notes
  surface. Pet controls stay in the separate pet console.

Current native transport:

- `MULTICLASS|window|clear`
- `MULTICLASS|profile|name=...|class1=...|class1_name=...|class2=...|class2_name=...|class3=...|class3_name=...|presentation=...|presentation_name=...|base=...|base_name=...|pets=...|locked=...|reweaves=...`
- `MULTICLASS|vitals|mana=...|max_mana=...|endurance=...|max_endurance=...|class_mask=...|aa_mask=...|presentation=...|base=...`
- `MULTICLASS|trio|roles=...|resonance=...|summary=...`
- `MULTICLASS|skills|summary=...|unlocked=...`
- `MULTICLASS|bonuses|summary=...`
- `MULTICLASS|selection|can_choose=...|status=...`
- `MULTICLASS|spell_levels|begin`
- `MULTICLASS|spell_level|id=...|level=...|presentation=...`
- `MULTICLASS|spell_levels|end|count=...`
- `MULTICLASS|pet_roster|mode=live|count=...|limit=...|focus=...|policy=native-ui|control=...`
- `MULTICLASS|pet|id=...|name=...|hp=...|mana=...|target=...|taunt=...|hold=...|spellhold=...|order=...|focused=...`
- `MULTICLASS|melody|has_bard=...|status=...`
- `MULTICLASS|melody_slot|slot=...|id=...|name=...|level=...|state=...`
- `MULTICLASS|melody_song|id=...|name=...|level=...|allowed=...|reason=...`
- `MULTICLASS|melody_songs|end|count=...`
- `MULTICLASS|disciplines|clear`
- `MULTICLASS|disciplines|summary|status=...|count=...|usable=...|ready=...`
- `MULTICLASS|discipline|slot=...|id=...|name=...|level=...|timer=...|total=...|ready=...|state=...`
- `MULTICLASS|disciplines|end|count=...`
- `MULTICLASS|window|show`
- `MULTICLASS|pet_window|show`
- `MULTICLASS|melody_window|show`
- `MULTICLASS|discipline_window|show`

Pet console target:

- The separate `NativeMulticlassPetWnd` shows up to three pet rows with focus
  marker, order, HP/mana, and taunt/hold/no-cast flags. The target column is
  intentionally omitted for now because roster target data only refreshes when
  the server sends a new pet snapshot. The window is resizable and is released
  with the rest of the Multiclass native surfaces during client UI reset/logout,
  then recreated on the next `/mcpets` or server snapshot.
- Selecting a row makes that pet the native-console focus, without requiring the
  player to target the pet in the world.
- Broad buttons broadcast attack, back off, follow, guard, and health to all
  active roster pets.
- Focused buttons apply taunt, sit, hold, no-cast/spell-hold, and leave/dismiss
  to the native-console focused pet.
- The stock pet window should remain a compatibility fallback only. The native
  Multiclass pet console is the intended complete control surface.

## Multiple-Pet Direction

Multiple full pets are intended for pet-heavy trios. The design should preserve
the familiar THJ feel while making the implementation cleaner:

- Do not implement multi-pet support by repeatedly rotating `Mob::SetPet()`.
  The existing `SetPet()` path detaches the previous pet from the owner.
- Add a real server-owned pet roster for Multiclass characters.
- Let summon spell effects create another owned pet only when the Multiclass
  roster has capacity; full rosters and non-Multiclass characters keep the
  normal one-pet denial.
- Treat Multiclass roster pets as valid owned pets even when another roster pet
  is the stock `client->petid`; otherwise normal EQEmu owner resolution clears
  older pets as ownerless.
- Keep one focused pet in the Multiclass roster for per-pet native UI actions.
- Targeting one of your own Multiclass pets may optionally promote focus later,
  but it is no longer the required control path.
- Broadcast broad commands such as attack, back off, follow me, guard me, and
  health report to all active Multiclass pets by default.
- Apply individual commands such as taunt, hold, greater hold, spell hold, sit,
  stand, and dismiss to the native-console focused pet by default.
- The separate native Multiclass pet window should become the complete pet
  console, showing each pet as a compact row with health, mana, taunt, hold,
  no-cast/spell-hold, stance, focus, and dismiss controls.
- Pet zoning, saving, loading, group-buff inclusion, pet affinity, pet buffs,
  pet inventory, and pet command state all need roster-aware handling. The
  current persistence slice saves secondary active pets in reserved stock pet
  slots `10..12`, and group/raid pet-affinity paths now include secondary
  non-charmed roster pets wherever stock logic already allows pets. Cross-zone
  and world-wide spell apply/fade helpers also fan out to secondary roster pets
  when the stock `AllowCrossZoneSpellsOnPets` rule is enabled. The dedicated
  `custom_multiclass_pet_state` table stores command metadata that does not fit
  stock pet rows.

## Implementation Slices

1. Add capability helpers and admin diagnostics.
2. Prove Warrior / Magician / Wizard mana, scribing, memorizing, casting,
   spell-group ranking, and fizzle behavior.
3. Add item eligibility and skill-cap behavior through the capability layer.
4. Add discipline and AA eligibility.
5. Prove client UI presentation for melee-base caster trios and caster-base
   discipline trios.
6. Add native UI trio selection and lock flow. Initial native bridge is live;
   richer class info and trio presentation are later polish.
7. Add multi-pet roster support, native-console focused-pet behavior, pet
   command routing, zoning/logout persistence, and group/raid pet-affinity
   plus cross-zone/world-wide spell inclusion for secondary roster pets.
8. Add Bard Melody as a separate native window so Bard songs can pulse without
   occupying normal spell gems.
9. Add trio resonance naming and level 10 through 60 passive bonuses.
10. Add rare one-slot reweave scaffolding.
11. Expand and tune the exact trio library, balance numbers, and native
    presentation after manual validation.

## Open Proof Points

- The target client exposes spellbook, casting controls, mana HUD, and the stock
  spell-gem window when the server sends a caster presentation class for a
  melee-base caster trio. Current tradeoff: stock inventory class text also
  reflects that presentation class.
- The stock target client rejects discipline controls for caster-base characters
  that gain melee classes. `NativeMulticlassDisciplineWnd` is the current
  fallback; remaining proof is learned-disc display/use coverage across the
  full trio matrix.
- Which client-side item restrictions still appear when the server allows gear
  through the trio mask? The server-side direct equipment move now allows any
  trio class match, and item packets now expand trio-usable item class masks for
  that Multiclass client. Testing should confirm whether the stock client still
  has any hard local drag/drop checks outside item packet data.
- How much AA packet shaping is needed for off-base-class AA display and
  purchase, especially after class-based cast restrictions now honor trio masks?
- Whether the stock AA window has a practical visible-entry cap for three-class
  profiles, and which expansion/rank filters give the cleanest middle ground.
- Which commands should always broadcast to all roster pets, and which should
  remain native-focused-pet only?
- Whether the server-authored vitals patch is enough to make the stock Player
  Window mana gauge appear for every melee-base caster trio, or whether a small
  separate native vitals HUD is needed as the reliable presentation surface.
- Whether secondary pet command mode details beyond taunt, hold, spell-hold, and
  order need deeper persistence than the current supplemental pet-state table
  provides.
- Whether Bard Melody should eventually allow targeted/control songs, and what
  targeting rules those songs need.
- Whether the first resonance bonus numbers are fun without flattening item,
  AA, or encounter tuning. Current tuning source is
  `MulticlassManager::ApplyTrioBonuses`.
- Whether the reweave bridge needs additional limits before player-facing UI,
  such as token checks, cooldowns, quest completion, or sanctuary-only NPCs.

## Operator Notes

Build and validate this branch through the feature workspace scripts:

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\verify-feature.ps1 multiclass
.\install-server-runtime.ps1 multiclass
.\install-client-files.ps1 multiclass
.\run-db-updates.ps1 multiclass
.\validate-install.ps1 multiclass
```

External patcher feed deployment is generated from the patcher project:

```powershell
cd D:\Codex\Apps\EQEmu-feature-patcher\features\patcher\eqemupatcher\service
.\New-WorkspacePatcherDeployment.ps1 -Project multiclass -BaseUrl http://<patch-host>:8091/patcher/
.\Test-WorkspacePatcherDeployment.ps1 -Project multiclass -BaseUrl http://<patch-host>:8091/patcher/
```

The `-Project` value is the workspace install `id` from
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json`. It usually matches the
feature id but should be verified before deployment. For this checkout, the
current install id is `multiclass`, so the published feed is
`http://<patch-host>:8091/patcher/multiclass/`.

Manual test coverage for the next pass is tracked in
`features/multiclass/VALIDATION.md`.

Deployment/testbed handoff notes for this project are tracked in
`docs/testbed-deployment-notes.md`.
