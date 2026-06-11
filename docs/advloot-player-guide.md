# Advanced Loot — Player Guide

Most of Advanced Loot is driven from the window (`#advloot window`), but
every button has a chat command behind it. Quick reference below.

## The Basics

- `#advloot` or `#advloot status` — show your current Advanced Loot settings.
- `#advloot window` — open the Advanced Loot window (also `ui` / `panel`).
- `#advloot on` / `#advloot off` — enable or disable Advanced Loot for your character. When off, corpses loot the old-fashioned way.
- `#advloot help` — command list in-game.

## How loot flows

When something in your group/raid kills a mob, its items go into the
Advanced Loot window instead of sitting on the corpse:

- **Personal list** — items only you were offered. Loot them, leave them, or set a filter so the choice happens automatically next time.
- **Shared list** — items the whole party rolls on or the Master Looter distributes. Vote **ND** (Need), **GD** (Greed), or **NO** (pass) any time — the first vote starts a 3-minute timer. When everyone has voted (or time runs out), Need beats Greed and the winner is rolled at random from the highest pool.
- Your saved **AN / AG / NV** filters vote for you automatically. If everyone in the party has the item filtered, the roll resolves instantly at kill time.
- Items nobody claims stay in the window until the corpse rots.

## Shared-loot actions (`#advloot action [Entry ID] ...`)

The Entry ID shows in the window; you'll rarely type these by hand.

- `need` / `greed` / `no` — cast your vote on a shared item.
- `alwaysneed` / `alwaysgreed` — vote AND save that choice as a filter for this item.
- `never` — pass and never show this item again (personal: leaves it and filters it).
- `loot` / `leave` — take or skip a personal item (or grab a Free Grab shared item).

**Master Looter only:**
- `ask` — start a Need/Greed vote for everyone ("?" button).
- `roll` — end the vote now; anyone who hasn't voted counts as a pass.
- `freegrab` — open the item up so any eligible member can just take it.
- `give [Character Name]` — hand the item to a specific member.

## Master Looter

- `#advloot master set [name]` — make someone the Master Looter (group/raid leader or the current Master Looter only). No name = yourself.
- `#advloot master clear` — remove the designation (falls back to an automatic pick).
- `#advloot masterlooter on|off` — opt in/out of being auto-picked as Master Looter.

## Filters (the AN / AG / NV / AR lists)

Set filters from the loot window checkboxes, the right-click menu, or the
Edit Filters window. By command:

- `#advloot filter list` — list your saved filters.
- `#advloot filter set [Item ID] [always_need|always_greed|never|unset]` — set or clear a filter.
- `#advloot filter autoroll [Item ID] [on|off]` — AR flag: when YOU are Master Looter, this item starts its Need/Greed ask automatically.
- `#advloot filter remove [Item ID]` — delete a filter.
- `#advloot applyfilters on|off` — master switch; off means your filters stop voting/looting for you.

Crafted/generated ("live") items filter by their base item — one filter
covers every roll of that base.

**Sharing filters:**
- `#advloot filter share` — offer your whole filter list to your target (they get 5 minutes to accept).
- `#advloot filter accept merge` / `accept replace` — take a pending offer; merge adds to yours, replace overwrites.
- `#advloot filter copyfrom [Character Name]` — copy filters from another character on your account (works while they're offline).

## Quality-of-life toggles (all `on|off`)

- `#advloot autoshow` — pop the loot window when new loot needs your attention.
- `#advloot shownew` — only auto-show for items your filters DON'T already cover.
- `#advloot autosplit` — auto-split corpse coin with the party.
- `#advloot autolootall` — automatically loot everything you're awarded.
- `#advloot autoremovelore` — drop a filter automatically once you loot that lore item.
- `#advloot confirmremove` — ask before removing filters from the filter window.

## Everything else

- `#advloot personal lootall` / `leaveall` — sweep your whole personal list (the Loot All / Leave All buttons).
- `#advloot inspect [Entry ID]` — preview an item in the window.
- `#advloot manage [Entry ID]` — open the Manage Loot window (votes, roll, give).
- `#advloot corpse target [Entry ID]` — target the corpse an item came from.
- `#advloot corpse link [Entry ID]` — link everything on that corpse into chat.
- `#advloot debug on|off` — chatty diagnostics if you're reporting a bug.
