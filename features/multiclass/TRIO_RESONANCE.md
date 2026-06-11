# Multiclass Trio Resonance

This file tracks the first implemented pass of trio names and level-gated
resonance bonuses. The goal is not restraint; it is a trio-first power fantasy
with enough structure that the server can tune it cleanly later.

## Bonus Cadence

Locked trios unlock one resonance tier every ten levels:

| Level | Tier |
| --- | --- |
| 10 | Tier 1 |
| 20 | Tier 2 |
| 30 | Tier 3 |
| 40 | Tier 4 |
| 50 | Tier 5 |
| 60 | Tier 6 |

Bonuses are passive and server-side. They are applied through
`MulticlassManager::ApplyTrioBonuses` during client bonus calculation and are
reported in the native Multiclass window through `MULTICLASS|bonuses|...`.

## Exact Trio Identities

| Classes | Name | Tier Names |
| --- | --- | --- |
| Magician / Necromancer / Beastlord | Grand Menagerie | Bonded Leashes, Pack Savagery, Triune Ward, Shared Command, Alpha Chorus, Grand Menagerie |
| Warrior / Wizard / Magician | Warbound Arcanum | Arcane Bulwark, Summoner's Guard, Battlecaster's Focus, Elemental Reprisal, War-Mage Tempo, Arcanum Ascendant |
| Wizard / Rogue / Berserker | Spellblade Compact | Hidden Edge, Volatile Footwork, Spellsteel Accuracy, Rupture Window, Executioner's Spark, Compact Fulfilled |
| Cleric / Paladin / Bard | Radiant Hymn | Aegis Verse, Merciful Cadence, Consecrated Guard, Bright Refrain, Unbroken Chorus, Radiant Hymn |
| Enchanter / Bard / Rogue | Velvet Conspiracy | Soft Step, Mesmeric Rhythm, Knife in the Song, Velvet Escape, Mindbreak Flourish, Perfect Conspiracy |
| Warrior / Rogue / Wizard | Arcane Duelist Pact | Arcane Guard, Opening Cut, Riposte Spark, Duelist's Read, Killing Tempo, Pact of the Last Word |
| Cleric / Druid / Shaman | Verdant Synod | Green Accord, Cleansing Root, Patient Waters, Triune Recovery, Living Bastion, Verdant Synod |
| Warrior / Cleric / Paladin | Aegis Covenant | Shield Oath, Interlocking Plate, Mercy Under Fire, Blessed Rampart, Last Stand Prayer, Aegis Covenant |
| Shadow Knight / Bard / Necromancer | Dirge of the Grave | Grave Note, Dread Cadence, Bone Chorus, Umbral Recovery, Funeral Engine, Dirge of the Grave |
| Ranger / Druid / Beastlord | Wildcall Pact | Trail Sense, Pack Step, Living Thorn, Primal Recovery, Wildfire Hunt, Wildcall Pact |

## Fallback Identity Rules

Exact trio identities are preferred. Other combinations fall into broad
archetypes so every locked trio still has meaningful flavor:

| Archetype | Trigger | Tier Theme |
| --- | --- | --- |
| Menagerie Mastery | Two or more pet classes | Pet durability, pet attack, pet criticals, command strength |
| Chorus of Three | Bard present | Song range, song mod cap, instrument support, mobility |
| Covenant Prime | Tank plus healer | HP, AC, mitigation, healing stability |
| Striker's Revelation | Striker plus caster | Accuracy, spell damage, Backstab/Frenzy pressure |
| Concord Ascendant | Healer plus caster | Mana recovery, healing, casting stability |
| Triad Ascendant | Everything else | Shared reserves, core stats, broad class harmony |

## Current Mechanical Bonus Families

The first implementation keeps bonuses intentionally broad and tunable:

- Core locked-trio bonuses: HP, AC, ATK, and class-resource reserves.
- Tank bonuses: HP, AC, shielding, and shield block.
- Healer bonuses: heal amount, heal rate, heal crit chance, and mana regen.
- INT caster bonuses: spell damage, spell crit chance, and mana.
- WIS caster bonuses: WIS, heal amount, and mana regen.
- Striker bonuses: ATK, accuracy, melee crit chance, Backstab/Frenzy damage.
- Pet bonuses: pet max HP, pet attack, pet crit, and pet mitigation.
- Bard bonuses: singing/instrument mods, song range, and song mod cap.

Exact identities add a small extra accent on top of those families. For
example, Grand Menagerie adds stronger pet bonuses and enables pet group-target
support at tier 4, while Spellblade Compact adds more Backstab/Frenzy damage
and casting stability.

## Tuning Notes

This is a first power pass. If the bonuses overshoot after testing, tune the
numbers in `MulticlassManager::ApplyTrioBonuses` before cutting features.
Bard Melody balance should stay separate; a future global Bard song scalar can
be added if the eight-song playstyle proves too strong.
