# Custom Server Vision

This document captures the early design direction for the new EQEmu-based custom server. It is meant to be a reference point for future chats, planning, source edits, balance work, and feature design.

The goal is not to lock the server into hard limits yet. The goal is to preserve the creative direction, remember the lessons from the previous server, and give future systems a shared design language.

## Starting References

- Base platform: `D:\Codex\Apps\Everquest`
- Previous custom scripts reference: the archived custom quest/script checkout.
- Especially useful old-script references:
  - `quests\plugins`
  - `quests\global\global_player.pl`
  - `quests\global\global_npc.pl`

The previous server had popular ideas and a lot of useful experiments, but many systems were implemented as always-on global scripts. That made content scaling, power creep, and cross-system interactions hard to reason about. For this server, the intent is to keep the creativity while moving more reusable systems into source code, database tables, rules, and structured config.

## Core Fantasy

Build an EverQuest server that feels like an alternate expansion timeline: familiar EQ bones, but with modern progression loops, challenge modes, quality-of-life systems, and class identity borrowed from games like Diablo, Final Fantasy, Dark Souls, World of Warcraft, Dungeons and Dragons Online, and other ARPG/MMO systems.

The server should support solo and boxing play first. Grouping should be rewarding, but not mandatory. EQEmu population can swing from a handful of players to hundreds, so progression should not depend on always finding people at the same level, tier, or flag state.

## Design Pillars

### Solo And Box Friendly

Players should be able to make meaningful progress alone or with boxes/bots. Harder group-style content can exist, but core progression should not collapse when the population is low.

### Modern Progression Inside EQ

Use familiar modern game loops:

- Diablo-style loot chase, affixes, difficulty scaling, and repeatable rewards.
- Final Fantasy-style job/class experimentation and long-term character investment.
- Dark Souls-style danger, deliberate encounters, cursed items, and meaningful risk.
- World of Warcraft-style talent/perk trees, mastery, and build expression.
- Dungeons and Dragons Online-style difficulty selection for replayable content.

### Source-Backed Systems, Scripted Content

Source code should own reusable mechanics and math:

- Zone difficulty modes.
- NPC scaling profiles.
- Auto loot, AOE loot, and auto sell.
- Gear score or power score.
- Perk trees.
- Rebirth progression.
- Attunement.
- Title stat bonuses.
- Epic frameworks.
- Reward budgets.

Scripts should mostly trigger content:

- Boss events.
- Quest flavor.
- Zone-specific encounters.
- Dialogue.
- One-off event logic.

### Power Budget Awareness

The previous server had many independent systems adding power at the same time. This time, major systems should be visible, inspectable, and eventually measurable. If a system affects player power, NPC power, rewards, currency, or loot, it should have a known role in the overall progression model.

This does not mean boring balance. It means we should know why something is wild.

## Feature Concepts

### Rebirth 2.0

Rebirth was one of the most popular features on the previous server and should be expanded rather than discarded.

Old behavior:

- Players could reset level after meeting requirements.
- Rebirth granted currency or milestone rewards.
- Rebirth encouraged class changing.
- Class changing tied into class mastery and long-term replay.

New direction:

- Rebirth grants perk points.
- Perk points feed into one or more talent-tree-like systems.
- Rebirth should encourage experimenting with classes, zones, builds, and items.
- Rebirth rewards should emphasize options and identity more than raw stat inflation.
- Rebirth should interact with class mastery, Voltron epics, attunement, titles, and account progression.

Potential source systems:

- Character or account rebirth state.
- Perk-point ledger.
- Perk tree definitions.
- Diminishing returns and anti-runaway math.
- Rebirth milestones by class.
- Account-wide unlock tracking.

Open design questions:

- Is rebirth per character, account-wide, or both?
- Should class changes be free, earned, or tied to rebirth?
- Should rebirth remove, preserve, or partially convert gear power?
- Should perk trees be class-specific, global, or hybrid?

### Perk System

The perk system can be the main long-term customization layer.

Potential structure:

- Rebirth grants perk points.
- Perks are spent in talent trees.
- Trees can represent combat style, utility, loot, survival, pets, class mastery, crafting, or challenge modifiers.
- Some perks may require achievements, title unlocks, class completions, attuned items, or epic components.

Possible tree examples:

- Slayer: damage, execute effects, elite mob bonuses.
- Survivor: mitigation, recovery, death protection, flask-like cooldowns.
- Treasure Hunter: loot radius, auto sell quality, currency bonuses, rare detection.
- Riftwalker: dungeon modifiers, affix rewards, challenge-mode bonuses.
- Commander: bot and pet support.
- Class Savant: cross-class mastery bonuses.

Important design note:

Perks should avoid becoming a flat permanent stat mountain. The best perks change how a player plays, farms, survives, loots, or builds.

### Normal, Hard, And Insane Zones

Inspired by Dungeons and Dragons Online difficulty selection.

Goal:

Let players replay many zones at relevant difficulty and rewards instead of abandoning old zones forever.

Possible model:

- Normal: baseline experience and loot.
- Hard: stronger mobs, better rewards, higher named chance, possible extra mechanics.
- Insane: major stat scaling, affixes, improved rewards, special currencies, rare unlocks.

Potential source systems:

- Difficulty selection for instances.
- Zone scaling profiles.
- Reward multipliers.
- Named spawn rate modifiers.
- Lockout or completion tracking.
- Difficulty-based achievements and titles.
- Gear score recommendations or requirements.

Design note:

This should become a core spine for the server. Once difficulty modes exist, many other systems can plug into them: attunement speed, rebirth tasks, perk unlocks, Voltron epic components, title progression, and named events.

### Auto Loot, AOE Loot, And Auto Sell

The previous server's auto loot and auto sell script was highly popular because it added convenience to an older EQ ruleset.

New direction:

- Move the core behavior into source code.
- Make it faster, safer, and less chat-command-driven.
- Provide a clearer UI or command interface.

Potential features:

- AOE loot within a configurable radius.
- Auto loot currencies.
- Auto sell trash based on filters.
- Keep, sell, destroy, and announce filters.
- Per-character or account-wide loot profiles.
- Group loot compatibility.
- Rarity or item-stat filters.
- Optional UI window if feasible through available client/opcode support.
- Fallback slash commands if UI is limited.

Design note:

This is a strong early source-edit candidate because it has immediate player-facing value and reduces script complexity.

### Class Changing And Class Mastery

The previous server encouraged class changing through rebirth and mastery. This should stay, but become more structured.

Goals:

- Let players experience many classes without feeling punished for leaving one behind.
- Reward class mastery without making every character eventually identical.
- Give each class more ways to shine.

Potential systems:

- Class completion milestones.
- Mastery rewards for reaching level, epic milestones, or challenge clears on a class.
- Class-specific perk branches.
- Account or character class history.
- Cross-class passive unlocks with strong caps.
- Content tags that let certain classes excel.

Examples of class shine:

- More undead zones for Paladins, Clerics, and Necromancers.
- Headshot-friendly humanoid content for Rangers.
- Assassinate-friendly targets for Rogues.
- Summoned or construct-heavy zones for specific counters.
- Caster-heavy areas where interrupts, mana drains, and resists matter.
- Pet-focused encounters for pet classes.

### Voltron Epics

Original idea:

Reward players for completing epics across multiple classes, especially because class changing made old epics unusable.

New direction:

Instead of a simple rank ladder, epics could become modular long-term artifacts assembled from class achievements.

Possible model:

- Each class has an epic core or component.
- Completing a class journey unlocks that component permanently.
- A Voltron epic combines earned components into a custom item.
- Components could grant selectable traits rather than only raw stats.
- The item might change based on active class, chosen module, or attunement state.

Potential component types:

- Core: class identity.
- Edge: damage style.
- Guard: defensive trait.
- Focus: spell or heal modifier.
- Relic: utility or proc.
- Catalyst: special build-defining effect.

Design note:

This is a good way to honor time spent in old classes without forcing raw power creep from every previous epic.

### Attunement System

Inspired by Synestria's gear mastery concept and EQ evolving items.

Concept:

When a player wears gear and gains experience, each equipped item gains its own experience. Once mastered, the player permanently absorbs a portion of that item's stats or unlocks a related bonus.

Why it fits EQ:

- EQ is already a gear-grind game.
- It gives value to old zones and forgotten items.
- It creates a collection loop: find, wear, master, archive.
- It supports solo/box progression without requiring constant raid availability.

Potential features:

- Equipped items gain attunement XP from kills, tasks, or challenge completions.
- Mastered items grant a small permanent stat, perk, title progress, or collection credit.
- Different item slots may have different mastery rules.
- Rarer items may grant unique unlocks instead of larger raw stats.
- Attunement may feed gear score, title unlocks, perk requirements, or Voltron epic components.

Balance notes:

- Permanent gains should probably be small, capped, categorized, or converted into rating.
- The system should reward breadth without making old players impossible to balance around.
- It may be safer to grant points, tags, or unlocks instead of directly inheriting full item stats.

Open design questions:

- What percentage of item stats can become permanent?
- Should mastered items be consumed, bound, archived, or simply marked?
- Should duplicate item mastery be blocked?
- Should every item count, or only curated custom/rare items?

### Gear Score Or Power Score

A gear score system could help the server understand player power.

Possible uses:

- Recommend zone difficulty.
- Gate or warn for Hard/Insane modes.
- Scale rifts and dynamic instances.
- Tune rewards.
- Compare balance outliers.
- Feed matchmaking or group suggestions if population allows.

Design note:

This may be more useful as an internal power score than as a public leaderboard number. It should account for gear, augments, attunement, perks, rebirth, class mastery, pet power, and possibly bots.

### Titles Give Stats

Titles can become a lightweight build and achievement system.

Possible model:

- Titles unlock from achievements, class mastery, zone clears, named kills, crafting, events, or attunement collections.
- Active title grants a small stat bonus, utility bonus, or system modifier.
- Only one title bonus is active at a time, preventing unlimited stacking.

Examples:

- Undead Slayer: bonus versus undead.
- Riftwalker: bonus rewards in difficulty instances.
- Treasure Hunter: improved loot radius or rare detection.
- Dragonsbane: minor bonus versus dragons.
- The Attuned: improved item mastery rate.

### Global Buffs With Upkeep

Inspired by THJ-style global buffs and upkeep.

Possible model:

- Players, guilds, or the whole server can fund global buffs.
- Buffs consume platinum, donator currency, or special currencies over time.
- Buffs can act as money sinks and community goals.

Possible buff scopes:

- Personal.
- Account.
- Guild.
- Zone.
- Server-wide.

Design note:

This could be a strong economy stabilizer if costs are meaningful and rewards are convenient rather than mandatory.

### Namedfest

Original idea:

Players pay or earn a reward that transforms a zone into a named-heavy event.

Possible model:

- Activate in an instance.
- Replace some or all mobs with named variants.
- Add affixes, loot bonuses, and risk modifiers.
- Duration-limited or clear-limited.
- Could be tied to Hard/Insane mode, special currency, or event tokens.

Design note:

This has great "chaotic fun" potential, but should probably run in instances rather than live public zones.

### Newbie Loot

Original idea:

Query by class and award a random useful starter item.

Possible model:

- On first login, class change, or tutorial completion, award a curated class-appropriate item.
- Could use weighted pools by class, role, and starting path.
- Could include a cursed/evolving starter item.

Design note:

This is a small feature but can set the tone early. It should feel like the server immediately has its own identity.

### Evolving Cursed Item

Wishlist idea:

An evolving item starts with negative stats and eventually turns into something valuable or can be turned in at max level.

Possible model:

- The item begins cursed, with drawbacks.
- It gains XP from kills, deaths survived, bosses, or challenge clears.
- At maturity, the player chooses to purify, corrupt, shatter, or turn it in.
- Different outcomes could feed perk points, titles, attunement, or an epic component.

Design note:

This fits the Dark Souls inspiration well. The item should ask the player to make a tradeoff, not just tolerate weak stats for a later prize.

### LDON Dungeons Reuse

LDON dungeons are a strong fit for repeatable custom systems.

Possible uses:

- Challenge-mode dungeons.
- Daily or weekly missions.
- Rogue-lite dungeon runs.
- Affix dungeons.
- Rebirth trials.
- Class mastery trials.
- Attunement farming.
- Voltron epic component hunts.

Design note:

LDON content can give the server a repeatable endgame without relying only on traditional raid zones.

## Feature Architecture Bias

Prefer central frameworks over scattered scripts:

- One difficulty engine instead of per-zone stat scripts.
- One reward profile system instead of many hand-coded loot rolls.
- One epic framework instead of 16 unrelated epic scripts.
- One perk system instead of many hidden permanent bonuses.
- One power-score model instead of guessing player strength.
- One auto-loot system in source instead of chat-heavy quest scripts.

Scripts can still be expressive, funny, and content-rich. They should not be forced to carry the whole mechanical design.

## Early Source Edit Candidates

Strong first candidates:

1. Auto loot, AOE loot, and auto sell core.
2. Difficulty-mode instance support: Normal, Hard, Insane.
3. Gear score or power score calculation.
4. Rebirth state and perk-point ledger.
5. Title stat bonus framework.
6. Attunement XP tracking for equipped items.
7. Central custom reward profile system.

Likely later candidates:

1. Voltron epic framework.
2. Full perk tree UI/data model.
3. NPC affix system.
4. Class mastery framework.
5. LDON challenge dungeon generator.
6. Telemetry for damage, healing, kill times, currency, and rewards.

## Old Server Lessons

The previous server's biggest problems were content scaling and power creep.

Observed risks:

- Too many global scripts modified combat and rewards independently.
- Tier scaling, pet scaling, epic ranks, rebirth rewards, treasure goblins, fabled loot, currency rewards, and item scripts could all stack at once.
- Large numbers became hard to manage.
- Scripted systems were difficult to inspect as a total balance model.
- One-off fixes tended to create more hidden interactions.

New approach:

- Keep the wild ideas.
- Centralize the math.
- Make player power inspectable.
- Let difficulty and rewards be configured through shared systems.
- Use scripts for flavor and content, not as the only engine.

## Working Principle

When adding a new feature, ask:

- Is this a reusable system or one piece of content?
- Does it affect player power, NPC power, loot, currency, or progression?
- Should this live in source, database/config, or scripts?
- How does it interact with rebirth, perks, attunement, difficulty, and class mastery?
- Can future us inspect, tune, or disable it cleanly?

The server should be allowed to get weird. It should just be weird on purpose.
