# Live Items Test Sheet

Thank you for helping test. You do not need to break things on purpose or write a perfect bug report. Just play through the steps, check the boxes that worked, and write down anything that felt wrong, confusing, missing, too strong, too weak, or weird.

Tester name:
Character name:
Class / level:
Date tested:

## Before You Start

- [ ] I patched my RoF2 client with the Live Items patcher.
- [ ] I could log in and enter `tutorialb`.
- [ ] The game did not show missing file, XML, or UI errors when starting.

Anything odd during login or patching:

## Meet The Test NPCs

Find and hail these NPCs:

- [ ] Vedra Forgecall answered my hail.
- [ ] Orin Augspinner answered my hail.
- [ ] Mavren Instancewright answered my hail.
- [ ] Talia Heirloomkeeper answered my hail.
- [ ] Nalyx Augmentweaver answered my hail.
- [ ] The clickable dialogue text worked when I clicked it.

Anything confusing about the NPC text:

## Tester Helper Commands

These are optional, but they help us trace bug reports.

- [ ] I used `#helditemid`, `#itemid`, or `#cursoritemid` while holding an item on my cursor.
- [ ] I used `#heirloomdebug` or `#evolvingdebug` while testing heirloom growth.

Useful command output or item IDs:

## Random Loot From Tutorialb Mobs

This is the main gameplay test. Kill normal mobs in `tutorialb` and loot them like you normally would.

- [ ] I killed at least 10 normal mobs.
- [ ] Each mob had a random Live Items item directly on the corpse.
- [ ] The item looked like normal loot, not a box or cache I had to open.
- [ ] I could loot the random items normally.
- [ ] Different mobs gave items with different names or stats.
- [ ] The items kept their special stats after zoning, camping, or relogging.
- [ ] Nothing crashed, duplicated, vanished, or got stuck on my cursor.

Best random item I found:

Worst or strangest random item I found:

Mobs I killed:

Notes:

## Item Forge Window

Talk to Vedra Forgecall and choose `open forge`.

- [ ] The Item Forge window opened.
- [ ] I could choose item types like weapon, armor, jewelry, charm, shield, or augment.
- [ ] I could enter a custom item name.
- [ ] I could change stats on the item.
- [ ] Creating the item gave me the item in game.
- [ ] The item name and stats matched what I entered.
- [ ] The stat step buttons worked when I changed the step size.
- [ ] The item kept its stats after zoning, camping, or relogging.

Item I created:

Did any stat field not work or look wrong?

Notes:

## Augment Test

Talk to Orin Augspinner. Say `start`, add the upgrades you want, optionally name the augment, then say `confirm`. Use `test shards` if you need Blood Shards for testing.

- [ ] I could start a shardwork augment.
- [ ] I could add the upgrades I wanted.
- [ ] I could name the augment.
- [ ] Orin showed the total Blood Shard cost.
- [ ] Orin spent Blood Shards when I confirmed.
- [ ] Orin gave me a custom augment.
- [ ] The augment had the stats I purchased.
- [ ] The augment kept its stats after zoning, camping, or relogging.
- [ ] I could insert the augment into normal equipment.
- [ ] The target item still looked usable and kept its normal slots after inserting the augment.

Augment item name/link:

Notes:

## Instance Upgrade Test

Talk to Mavren Instancewright. Hand him exactly one Live Items item, with no coins and no extra items.

- [ ] Mavren returned the same item as a `+1` version.
- [ ] Mavren returned only the upgraded item and did not also give the original item back.
- [ ] The item name changed to show the upgrade.
- [ ] The item's existing stats increased by 10 where it already had stats.
- [ ] The upgraded item kept its stats after zoning, camping, or relogging.
- [ ] Nothing duplicated, vanished, or got stuck on my cursor.

Item upgraded:

Notes:

## Evolving Heirloom Test

Talk to Talia Heirloomkeeper and say `heirloom`. Keep the item equipped or in your inventory, then gain a level if you can.

- [ ] Talia gave me a class-matched heirloom weapon.
- [ ] The heirloom had custom Live Items stats.
- [ ] `#heirloomdebug` or `#evolvingdebug` found the heirloom while it was equipped or in inventory.
- [ ] When I gained a level, the heirloom grew stronger.
- [ ] The heirloom kept its upgraded stats after zoning, camping, or relogging.

Heirloom item name/link:

Notes:

## Augment Fusion Test

Talk to Nalyx Augmentweaver. Say `catalyst` if you need an Augment Catalyst, then hand in exactly one Augment Catalyst and one to three augment items, with no coins.

- [ ] Nalyx gave me an Augment Catalyst when I asked for one.
- [ ] Nalyx accepted one catalyst plus one to three augments.
- [ ] Nalyx returned one fused augment, not multiple augments.
- [ ] The fused augment showed combined stats from the handed-in augments.
- [ ] The fused augment could be inspected normally.
- [ ] The fused augment kept its stats after zoning, camping, or relogging.
- [ ] Nothing duplicated, vanished, or got stuck on my cursor.

Fused augment item name/link:

Notes:

## General Feel

These questions are just as useful as bug reports.

What felt fun?

What felt annoying?

What was confusing?

Did any item seem way too strong or way too weak?

Would you want this kind of loot while leveling normally?

## Problems Or Bugs

If something broke, please write what happened in plain English. Screenshots and item links help, but they are not required.

Example: "I killed a kobold near the cave entrance. The corpse had no random item. I was level 5 warrior."

Problem 1:

What were you doing?

What did you expect to happen?

What actually happened?

Problem 2:

What were you doing?

What did you expect to happen?

What actually happened?

Problem 3:

What were you doing?

What did you expect to happen?

What actually happened?
