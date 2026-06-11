# Multiclass Manifest

Feature id: `multiclass`

This manifest lists the files and hook points that make up the standalone
Multiclass feature slice.

## Added Server Files

- `zone/multiclass_manager.cpp`
- `zone/multiclass_manager.h`
- `zone/gm_commands/multiclass.cpp`
- `zone/gm_commands/multiclass_ui.cpp`

## Existing Server Files To Patch

| File | Purpose |
| --- | --- |
| `common/database/database_update_manifest_custom.h` | Add the custom Multiclass schema migration. |
| `common/repositories/skill_caps_repository.h` | Add Multiclass client export support that emits highest-known skill caps for every player class. |
| `common/version.h` | Set `CUSTOM_BINARY_DATABASE_VERSION` for the custom migration. |
| `client_files/export/main.cpp` | Use the Multiclass-aware SkillCaps export for this standalone feature checkout. |
| `zone/CMakeLists.txt` | Build the Multiclass manager and command source. |
| `zone/aa.cpp` | Apply Multiclass shifted class masks to AA send/use checks and guard learned-AA packet writes against the fixed client profile array. |
| `zone/attack.cpp` | Apply Multiclass item usability to client weapon damage class validation and route class-specific combat skill bonuses through trio membership. |
| `zone/bonuses.cpp` | Apply Multiclass item usability to item bonus eligibility and apply locked-trio resonance passives during client bonus calculation. |
| `zone/client.cpp` | Apply Multiclass skill caps/training, class-derived innate abilities, tracking capability, max-skill handling, spell/disc auto-learn lists, native spellbook level refresh before bulk-scribe client packets, and secondary roster pets in generic spell-apply lists. |
| `zone/client_mods.cpp` | Apply Multiclass caster mana source and bard instrument logic. |
| `zone/client_packet.cpp` | Apply Multiclass bard item-click, rogue utility-skill actions, and item usability checks, seed eligible locked-trio skills on login/zone completion, send the caster-capable presentation class in the outbound player profile, and expand item preview/adventure merchant class-mask presentation. |
| `zone/client_process.cpp` | Apply Multiclass spell memorization, scroll scribing, skill trainer access, merchant class masks, bulk inventory item-packet class-mask presentation, and server-side Bard Melody pulses. |
| `zone/command.cpp` | Register `#multiclass`, `#multiclassui`, and `#mc`. |
| `zone/command.h` | Declare the command handlers. |
| `zone/effects.cpp` | Apply Multiclass spell level, scroll/tome, discipline, and spell modifier checks. |
| `zone/exp.cpp` | Seed newly eligible Multiclass class skills after level changes. |
| `zone/guild_mgr.cpp` | Apply Multiclass class masks to guild-bank usability display, including bulk list entries. |
| `zone/groups.cpp` | Include secondary Multiclass roster pets in group pet-affinity spell fanout and pet heal-count checks. |
| `zone/inventory.cpp` | Apply Multiclass item usability to direct equipment swaps and loot auto-equip, and expand per-client item-packet class masks for trio-usable items. |
| `zone/mob.cpp` | Apply Multiclass INT/WIS caster identity and keep Multiclass roster pets owner-valid even when they are not the stock active pet. |
| `zone/pets.cpp` | Allow eligible Multiclass profiles to keep up to three owned full pets without rotating `SetPet()`. |
| `zone/raids.cpp` | Include secondary Multiclass roster pets in raid-group pet-affinity spell fanout. |
| `zone/spell_effects.cpp` | Let eligible Multiclass pet trios pass the summon-spell one-pet guard until their roster limit is reached, and apply Multiclass trio masks to player-facing class cast restrictions. |
| `zone/special_attacks.cpp` | Apply Multiclass class membership to combat ability branches such as Backstab, Frenzy, Kick, Monk attacks, and Bard casting exceptions. |
| `zone/spells.cpp` | Apply Multiclass spell levels to fizzle and spell-group cache logic, item-click class restrictions, refresh native spellbook levels after scroll scribing, and include secondary roster pets in direct group-target pet-affinity casts. |
| `zone/worldserver.cpp` | Include secondary Multiclass roster pets in cross-zone and world-wide spell apply/fade pet paths. |
| `zone/zonedb.cpp` | Persist secondary Multiclass roster pets in reserved stock pet slots, save/load supplemental pet command state, and reload pets after the normal primary pet. |
| `zone/zonedb.h` | Expose the Multiclass secondary pet and pet-state reload hooks. |

## Database Objects

Custom migration version 1 creates the fixed trio profile schema:

- `custom_multiclass_profiles`
- `custom_multiclass_profile_audit`

Custom migration version 2 upgrades the earlier first-slice active-class
scaffold, if present, into `custom_multiclass_profiles` and removes the old
active-track tables.

Custom migration version 3 creates supplemental pet command-state storage:

- `custom_multiclass_pet_state`

Custom migration version 4 creates Bard Melody slot storage:

- `custom_multiclass_bard_melody`

Reference SQL lives in:

- `features/multiclass/sql/001_multiclass_schema.sql`

## Added Feature Docs

- `features/multiclass/TRIO_RESONANCE.md`

## Native Client Assets

| File | Purpose |
| --- | --- |
| `client_files/native_autoloot/ui/EQUI_NativeMulticlassWnd.xml` | Native SIDL window shell for Multiclass, Multiclass Pets, Bard Melody, and Multiclass Discs, including resize anchors, the stock active-discipline gauge mirror, and hotkey controls for sizable native windows. |
| `client_files/native_autoloot/eq-core-dll/eq-core-dll-visualstudio2022.sln` | Feature-local native DLL solution used by the Multiclass build. |
| `client_files/native_autoloot/eq-core-dll/src/eq-core-dll-vs2022.vcxproj` | Feature-local native DLL project file. |
| `client_files/native_autoloot/eq-core-dll/src/core_autoloot_native.h` | Feature-local native runtime implementation for Multiclass windows, transport parsing, command rewrites, spellbook patching, server-authored vitals/class presentation, player mana-HUD and spell-gem presentation, pet UI, Bard Melody UI, discipline UI, discipline reuse countdown display, discipline hotkey creation, and session cleanup. |
| `client_files/native_autoloot/eq-core-dll/bin/dinput8.dll` | Built feature-local runtime DLL installed into the Multiclass client. |
| `features/multiclass/patcher.yml` | Project-owned external/test-client patcher feed declaration. Maps feature-owned client files into the EverQuest client folder and requests generated `eqhost`, generated `EQUI.xml`, and the native XML include. |
| Target client `uifiles/default/EQUI.xml` | Must include `EQUI_NativeMulticlassWnd.xml` so the EQ client loads the window definition. |

Client patch syncing for external testers is owned by
`features/multiclass/patcher.yml`. Add any client-facing DLL, XML, config,
status, patch note, zone asset, or other test-client file there with a
repo-relative `source` and client-relative `destination`. Do not use the local
EQ client folder as the source of truth. For real external syncs, missing files
are release blockers; `-AllowMissingClientFiles` is only for partial local
testing. Patcher deployment `-Project` values come from
`D:\Codex\Apps\EQEmu-feature-workspaces\installs.json` install ids.

## Admin / Native Bridge Commands

- `#multiclass status`
- `#multiclass set <class1> <class2> <class3>`
- `#multiclass diag`
- `#multiclass diag skills`
- `#multiclass diag spells`
- `#multiclass diag discs`
- `#multiclass diag aa`
  - reports trio AA mask, visible AA definition count, learned profile entries,
    first-rank purchase candidates, grant-only entries, and learned-AA packet
    guard usage
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

`#multiclass` remains GMAdmin-only. `#multiclassui` and its shorter `#mc` alias
are Player-accessible so the native DLL can request snapshots and submit
server-authoritative trio choices. The native DLL also rewrites `/mc`,
`/multiclass`, and `/multiclassui` into `#mc` bridge calls, making `/mc` the
recognizable in-game reopen shortcut while keeping selection state on the
server. It also rewrites `/mcpets`, `/multiclasspets`, and `/petui` into the
standalone Multiclass pet console bridge. `/disc`, `/discs`, `/discipline`,
`/discwindow`, `/combatability`, `/combatabilities`, and `/mc disc` rewrite into
the native Multiclass discipline bridge so caster-root melee trios do not hit
the stock client class gate.
`#mc reweave <slot 2|3> <class>` is the first player-safe bridge for rare
one-slot reweaves. It consumes a granted reweave, requires a locked trio, and
is restricted to cities or bind-safe sanctuaries. Slot 1 remains tied to the
base character identity for now.

## Native Transport

- `MULTICLASS|profile|...`
- `MULTICLASS|vitals|...`
- `MULTICLASS|trio|...`
- `MULTICLASS|skills|summary=...|unlocked=...`
- `MULTICLASS|bonuses|summary=...`
- `MULTICLASS|selection|...`
- `MULTICLASS|spell_levels|begin`
- `MULTICLASS|spell_level|id=...|level=...|presentation=...`
- `MULTICLASS|spell_levels|end|count=...`
- `MULTICLASS|pet_roster|...`
- `MULTICLASS|pet|...`
- `MULTICLASS|melody|...`
- `MULTICLASS|melody_slot|...`
- `MULTICLASS|melody_song|...`
- `MULTICLASS|melody_songs|end|count=...`
- `MULTICLASS|disciplines|clear`
- `MULTICLASS|disciplines|summary|...`
- `MULTICLASS|discipline|...` with `timer` and `total` reuse fields
- `MULTICLASS|disciplines|end|count=...`
- `MULTICLASS|window|clear`
- `MULTICLASS|window|show`
- `MULTICLASS|pet_window|show`
- `MULTICLASS|melody_window|show`
- `MULTICLASS|discipline_window|show`

The server owns all future validation and state mutation. The native window is
only a display/input surface. This branch owns the native runtime code for
these Multiclass behaviors instead of depending on the shared
`native-client-runtime` checkout. The runtime parses these transport lines,
opens `NativeMulticlassWnd` for trio identity/selection, opens the separate
`NativeMulticlassPetWnd` for pet roster controls, refreshes the displayed trio,
opens the separate `NativeMulticlassMelodyWnd` for Bard Melody controls,
opens the separate `NativeMulticlassDisciplineWnd` for learned discipline use,
reuse countdown display, active-effect gauge mirroring, and hotkey creation,
uses compact slot selectors for unlocked profiles, renders server-owned
role/resonance notes, skill-unlock summaries, and resonance-bonus summaries,
patches the client spellbook and spell-gem right-click menu required-level display for
off-presentation-class trio spells using both spell-id and normalized-name
lookup keys, consumes server-authored
vitals/classmask snapshots, patches local mana/class presentation data for
melee-base caster trios, keeps the stock Player Window mana gauge/labels visible
for mana-class presentations, retries showing the stock spell-gem window after
profile/vitals refreshes, destroys stale native windows during client UI
reset/logout, and sends locked-trio choices back through `#mc choose`. Pet buttons
send targeted roster commands through `#mc pet` and bridge
through stock EQEmu pet command handling where possible so normal pet response
text and command-state updates are preserved. The slim pet console shows pet
name, mode, HP/MP, and taunt/hold/no-cast flags; the stale target column was
removed because it only updates on explicit roster refresh. The pet console is
resizable and is released with the other Multiclass native windows during client
UI reset/logout.
It also rewrites `/mcmelody`, `/multiclassmelody`, `/melodyui`, and `/mc
melody` into the Bard Melody bridge.
It rewrites `/disc` and related discipline shortcuts into `#mc disc`.

## Current Capability Hooks

- Trio class slots and class masks live in `zone/multiclass_manager.*`.
- Locked trio names, resonance keys, level 10 through 60 passive tiers, and
  rare one-slot reweave rules live in `zone/multiclass_manager.*`.
- Mana/caster identity uses selected caster slots while preserving the base
  profile class as the client identity.
- Spell, discipline, scroll, tome, fizzle, skill, combat ability, item equip,
  item-click, guild-bank, merchant, loot auto-equip, weapon damage, class cast
  restrictions, and AA class
  gates have first-pass Multiclass routing. Outgoing item packets, item previews,
  adventure merchant lists, and the login bulk-inventory snapshot clone or
  expand trio-usable item class masks to the character's trio, keeping global
  item data unchanged while reducing client-side drag/drop and tooltip mismatch.
  Native snapshots also send the best trio level for scribed off-class spells
  so the client spellbook does not show level 255 for usable Necromancer,
  Beastlord, or other non-presentation spells. The native runtime rewrites
  spell-gem context menu labels from the same patch rows, keyed by spell id when
  the context menu exposes it and by normalized spell name as fallback. Bulk
  spell scribing now sends those native level rows before the client receives
  the scribe packets so newly cached spell-gem context menus do not retain
  level 255 until zoning.
- The feature-local client export utility emits `SkillCaps.txt` with the
  highest known cap for each skill/level across all player classes, then relies
  on the server capability layer for authoritative per-trio skill enforcement.
- Locked profiles auto-seed eligible non-tradeskill, non-specialization class
  skills to `1` on trio lock, login/zone completion, level-up, and native
  refresh once the character meets the best trio train level. This unlocks
  off-base class tools without maxing skills or bypassing tradeskill/content
  progression.
- Eligible pet-heavy Multiclass profiles can keep up to three active owned pets
  in-zone. Summon spell effects now defer the stock one-pet denial when the
  Multiclass roster still has capacity, then `zone/pets.cpp` registers the new
  owned pet without rotating away the existing focused pet. `zone/mob.cpp`
  treats those roster pets as owner-valid even when they are not the stock
  `client->petid`, preventing older roster pets from being cleared as
  ownerless. Secondary roster pets now save into reserved `character_pet_*`
  slots `10..12`, preserving stock pet HP, mana, buffs, worn equipment, size,
  petpower, and taunt state across zoning and logout/login. Supplemental pet
  command metadata lives in `custom_multiclass_pet_state`. Group/raid
  pet-affinity spell fanout, generic spell-apply lists, and cross-zone/world-wide
  spell apply/fade pet paths include secondary roster pets wherever stock logic
  already allows pets. The separate native pet console displays roster rows,
  native focus, HP/mana, taunt, hold, and no-cast state, with
  broadcast/focused command routing.
- Bard Melody uses `custom_multiclass_bard_melody`, `#mc melody`, and
  `NativeMulticlassMelodyWnd` to pulse up to four selected scribed Bard songs
  without consuming spell gems. Slot changes preserve the active pulse cadence
  instead of refreshing the whole stack immediately. When Melody has active
  slots, Bard songs cast from normal spell gems are on-demand casts and do not
  start the stock auto-repeat loop. First pass blocks
  charm/fear/mez/stun/root/lull songs from Melody until they are explicitly
  validated.
- Caster-root melee trios use `#mc disc`, `/disc` rewrites, and
  `NativeMulticlassDisciplineWnd` to list and activate learned disciplines
  through server-authoritative Multiclass discipline checks when the stock
  combat window refuses the presentation class.
- `/mc` open/refresh and trio lock resend the stock AA table with the trio AA
  mask, giving the client another chance to display off-base class AA entries
  without a camp cycle.
- Locked profiles receive server-side resonance bonuses every ten levels from
  10 through 60. Exact trio identities get named tracks and accent bonuses;
  every other trio falls back to role-based tracks. Native snapshots send the
  current/next tier summary through `MULTICLASS|bonuses|...`.
- Rare reweaves have initial server-side scaffolding. Admins can grant
  reweaves, and players can spend one through the native bridge to change slot
  2 or 3 in a city or bind-safe sanctuary.

## Living Design Notes

- `docs/multiclass.md` records the current source-dive facts, fixed-trio
  decisions, capability-layer direction, multiple-pet policy, implementation
  slices, and open client proof points.
- `features/multiclass/VALIDATION.md` records the manual trio matrix and
  per-trio proof checklist for the next test pass.
- `features/multiclass/TRIO_RESONANCE.md` records exact trio names, fallback
  archetypes, and level 10 through 60 resonance tier names.

## Test Target

- Server: `D:\EQServers\EQServer-Multiclass`
- Client: `D:\EQClients\EQClient-Multiclass`
- Database: `eqemu_multiclass`
