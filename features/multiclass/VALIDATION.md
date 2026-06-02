# Multiclass Validation Matrix

## Current Pass Status

2026-05-26: the current manual validation matrix has been worked through in
game. The pass covered the first required trio set, native Multiclass windows,
multi-pet behavior, caster presentation, spell gems/book behavior, discipline
bridge behavior, and Bard Melody.

Issues found during the pass and fixed in this branch:

- Native discipline window resize/anchor bug hid the Use and Hotkey buttons.
- Bard Melody slot edits refreshed the whole song stack immediately.
- Bard songs cast from normal spell gems while Melody was active could stick in
  the stock auto-repeat loop. Melody now owns sustained song looping, while
  spell-gem Bard songs are on-demand casts when Melody has active slots.

Next validation should expand this document instead of repeating only the first
matrix. Candidate areas: deeper AA edge cases, item/equipment edge cases,
zoning/death/reconnect pet persistence, Bard balance tuning, and any trio
specific resonance/passive work.

## Second-Pass Slice Queue

The current overnight build is intended to be tested manually in slices. Do not
start or restart the server/client from automation unless the current test
slice explicitly asks for it.

| Slice | Area | Morning Validation Goal |
| --- | --- | --- |
| 1 | Startup/session cleanup | Log in, open `/mc`, `/mcpets`, `/mcmelody`, and `/disc`, then camp to character select and back without stale native windows or login crashes. |
| 2 | Caster presentation | Warrior-root caster trios should show mana, spellbook, and spell-gem bar after login without needing Alt+S or zoning. |
| 3 | Spell display | Scribe off-presentation spells and check spellbook, spell-gem right-click menus, merchant spell lists, and scroll previews for lingering level 255 labels. |
| 4 | Skills/class tools | Wizard / Rogue / Berserker and similar trios should seed Backstab, Frenzy, combat defenses, tracking/forage/pick tools where eligible, then skill up normally. |
| 5 | Disc bridge | Learn and use off-base disciplines through `NativeMulticlassDisciplineWnd`; verify active gauge, reuse timer, Use, and Hotkey behavior. |
| 6 | AA visibility | Open the stock AA window after `/mc refresh` and verify visible off-base AA entries, purchase, learned persistence, active use, timers, and passive effects. Record any apparent client cap. |
| 7 | Item gates | Equip, swap, click, augment, bandolier, trade/parcel/merchant/guild-bank, and preview items that match any trio class; confirm no-trio items still fail. |
| 8 | Multi-pet hardening | Summon 1/2/3 pets, command all/focused, zone, camp/relog, crash/reconnect if safe, die/revive, and confirm persistence/clearing without duplication. |
| 9 | Bard Melody | Run four Melody songs, cast extra Bard songs from normal gems, clear/swap slots, and confirm no instant refresh spam or stuck stock auto-repeat. |
| 10 | Trio resonance | Check level 10/20/30/40/50/60 tier summaries and stat changes for exact trios and fallback trios; tune if bonuses feel flat or excessive. |
| 11 | Reweave | Grant a reweave, attempt slot 1, attempt slot 2/3 outside a safe zone, spend one in a city/bind-safe zone, and verify profile, AA, skills, spells, pets, and native summary refresh. |

## New Resonance Test Trios

These exact identities have named level 10 through 60 tracks in the current
server pass:

| Trio | Identity | Validation Angle |
| --- | --- | --- |
| Magician / Necromancer / Beastlord | Grand Menagerie | Pet durability, pet pressure, tier 4 pet group-target support. |
| Warrior / Wizard / Magician | Warbound Arcanum | Melee-root caster presentation, tank shell, summoned pressure. |
| Wizard / Rogue / Berserker | Spellblade Compact | Caster-root melee tools, Backstab/Frenzy, disc bridge, spell burst. |
| Cleric / Paladin / Bard | Radiant Hymn | Healing, mitigation, Bard Melody support, hybrid tools. |
| Enchanter / Bard / Rogue | Velvet Conspiracy | Control, Melody, Rogue tools, on-demand Bard spell behavior. |
| Warrior / Rogue / Wizard | Arcane Duelist Pact | Tank/striker/caster blend and melee-plus-spell pressure. |
| Cleric / Druid / Shaman | Verdant Synod | Pure recovery and mana/heal scaling. |
| Warrior / Cleric / Paladin | Aegis Covenant | Defensive stacking and survivability. |
| Shadow Knight / Bard / Necromancer | Dirge of the Grave | Shadow spell damage, pet pressure, Bard support. |
| Ranger / Druid / Beastlord | Wildcall Pact | Tracking/nature tools, pet bonuses, hybrid casting. |

## First-Pass Checklist

This was the first manual test checklist. Do not start or restart the
server/client from automation unless the current test slice explicitly asks for
it.

## Required Trios

| Trio | Focus | Current Expected Proof |
| --- | --- | --- |
| Warrior / Magician / Wizard | Melee base with caster UI | Mana HUD appears, spellbook/gems work, one pet works, class cast restrictions accept Magician/Wizard spells. |
| Magician / Necromancer / Beastlord | Pet-heavy caster trio | Three full pets summon, command, save through zone/camp/relog, and restore hold/taunt/focus/order state. |
| Wizard / Rogue / Berserker | Caster base with melee tools | Backstab/Frenzy skills seed, combat ability packets work if stock UI exposes them, class-restricted AA/disc effects accept Rogue/Berserker. |
| Cleric / Paladin / Bard | Priest/hybrid/Bard mix | Bard Melody window opens, four eligible songs can be assigned, safe songs pulse, Paladin timers/discs remain available. |
| Enchanter / Bard / Rogue | Control/Bard/Rogue mix | Melody blocks unsafe control songs, Rogue tools seed, spell gems remain free for non-song casting. |

## Per-Trio Checks

- Login: `/mc` opens the main window and shows the expected trio, presentation
  class, skill summary, pet policy, and Melody button when Bard is present.
- Pets: `/mcpets` opens the separate pet console; Attack/Back/Follow/Guard and
  Health broadcast, while Taunt/Hold/Spellhold/Cast/Dismiss apply to the focused
  row.
- Pet persistence: with three pets up, zone, camp/relog, and crash/reconnect if
  safe; confirm all pets restore without duplication and that focus/hold/order
  state is retained.
- Spells: scribe, memorize, cast, fizzle, inspect spellbook levels, and inspect
  spell-gem right-click labels for off-presentation classes.
- Discs/class tools: learn/use disciplines and combat abilities for off-base
  classes; if stock buttons are missing, record the exact base/trio/client UI
  combination for the native Class Tools fallback.
- Items: equip/use/click an item allowed by any trio class and confirm items
  allowed by no trio class still fail.
- AA: display, purchase, activate, timer, passive bonus, and persistence for an
  off-base AA where possible.
- Bard Melody: assign up to four scribed safe songs, verify pulses without
  occupying spell gems, clear one slot, clear all slots, and confirm blocked
  control songs show a clear reason.

## Build/Install Flow

```powershell
cd D:\Codex\Apps\EQEmu-feature-workspaces
.\verify-feature.ps1 multiclass
.\install-server-runtime.ps1 multiclass
.\install-client-files.ps1 multiclass
.\run-db-updates.ps1 multiclass
.\validate-install.ps1 multiclass
```

After a working build, checkpoint feature-owned source and runtime files:

```powershell
git status --short
git add <feature-owned files>
git commit -m "Checkpoint Multiclass native client work"
```
