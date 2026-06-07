-- Public testbed seed content for Live Items / Item Forge.
-- This tester round focuses on direct live loot, the forge NPC, and augment rewards.

DELETE FROM spawn2 WHERE id = 900900 OR spawngroupID = 900900;
DELETE FROM spawnentry WHERE spawngroupID = 900900 OR npcID = 900900;
DELETE FROM spawngroup WHERE id = 900900;
DELETE FROM npc_types WHERE id = 900900;
DELETE FROM items WHERE id BETWEEN 900010 AND 900016;
DELETE FROM items WHERE id = 900090;
DELETE FROM items WHERE id BETWEEN 900101 AND 900116;

REPLACE INTO alternate_currency (id, item_id) VALUES
(90, 81436);

REPLACE INTO items (
  id, minstatus, Name, lore, comment, itemclass, itemtype, classes, races,
  norent, nodrop, size, weight, magic, loregroup, maxcharges, price, sellrate,
  questitemflag, updated, icon, stackable, stacksize, slots, damage, delay,
  hp, mana, ac, augslot1type, augslot1visible, augslot2type, augslot2visible,
  augtype, haste
) VALUES
(199091, 0, 'Live Items Test Loot Cache', 'Open this cache to roll a per-instance Item Forge test reward.', 'Seeded by Live Items testbed SQL', 0, 11, 65535, 65535, 255, 0, 1, 1, 1, 0, 0, 0, 1, 1, NOW(), 667, 1, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(199201, 0, 'Live Items Test Weapon Template', 'A base template for Live Items tester instance rolls.', 'Seeded by Live Items testbed SQL', 0, 0, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 519, 0, 0, 24576, 8, 24, 20, 20, 5, 7, 1, 7, 1, 0, 0),
(199202, 0, 'Live Items Test Armor Template', 'A base template for Live Items tester instance rolls.', 'Seeded by Live Items testbed SQL', 0, 10, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 538, 0, 0, 131072, 0, 0, 20, 20, 10, 7, 1, 7, 1, 0, 0),
(199203, 0, 'Live Items Test Jewelry Template', 'A base template for Live Items tester instance rolls.', 'Seeded by Live Items testbed SQL', 0, 29, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 674, 0, 0, 98304, 0, 0, 20, 20, 2, 7, 1, 7, 1, 0, 0),
(199204, 0, 'Live Items Test Charm Template', 'A base template for Live Items tester instance rolls.', 'Seeded by Live Items testbed SQL', 0, 52, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 1123, 0, 0, 1, 0, 0, 20, 20, 5, 7, 1, 7, 1, 0, 0),
(199205, 0, 'Live Items Test Shield Template', 'A base template for Live Items tester instance rolls.', 'Seeded by Live Items testbed SQL', 0, 8, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 535, 0, 0, 16384, 0, 0, 20, 20, 15, 7, 1, 7, 1, 0, 0),
(199206, 0, 'Live Items Test Augment Template', 'A base template for Live Items tester instance rolls.', 'Seeded by Live Items testbed SQL', 0, 54, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 646, 0, 0, 0, 0, 0, 10, 10, 3, 0, 0, 0, 0, 7, 0),
(199207, 0, 'Live Items Shardwork Augment Template', 'A base template for Orin shardwork augment rewards.', 'Seeded by Live Items testbed SQL', 0, 54, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 646, 0, 0, 8388607, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2147483647, 0);

REPLACE INTO npc_types (
  id, name, lastname, level, race, `class`, bodytype, hp, gender, texture,
  size, runspeed, findable, trackable, isquest, show_name, special_abilities,
  skip_global_loot
) VALUES
-- 900903 is reserved for the AI dialogue Sage Aurelian testbed NPC.
(900901, 'Vedra_Forgecall', 'Live Items Forge Tester', 70, 1, 1, 1, 100000, 0, 0, 6.0, 1.25, 1, 1, 1, 1, '19,1', 1),
(900902, 'Orin_Augspinner', 'Live Items Augment Tester', 70, 1, 1, 1, 100000, 0, 0, 6.0, 1.25, 1, 1, 1, 1, '19,1', 1),
(900906, 'Mavren_Instancewright', 'Live Items Instance Mutator', 70, 1, 1, 1, 100000, 0, 0, 6.0, 1.25, 1, 1, 1, 1, '19,1', 1),
(900904, 'Talia_Heirloomkeeper', 'Live Items Evolving Tester', 70, 1, 1, 1, 100000, 0, 0, 6.0, 1.25, 1, 1, 1, 1, '19,1', 1);

REPLACE INTO spawngroup (id, name, spawn_limit) VALUES
(900901, 'live_items_vedra_forgecall', 1),
(900902, 'live_items_orin_augspinner', 1),
(900906, 'live_items_mavren_instancewright', 1),
(900904, 'live_items_talia_heirloomkeeper', 1);

REPLACE INTO spawnentry (spawngroupID, npcID, chance) VALUES
(900901, 900901, 100),
(900902, 900902, 100),
(900906, 900906, 100),
(900904, 900904, 100);

REPLACE INTO spawn2 (
  id, spawngroupID, zone, version, x, y, z, heading, respawntime, variance
) VALUES
(900901, 900901, 'tutorialb', 0, -56.0, -80.0, 25.0, 128.0, 300, 0),
(900902, 900902, 'tutorialb', 0, -48.0, -80.0, 25.0, 128.0, 300, 0),
(900906, 900906, 'tutorialb', 0, -40.0, -80.0, 25.0, 128.0, 300, 0),
(900904, 900904, 'tutorialb', 0, -32.0, -80.0, 25.0, 128.0, 300, 0);
