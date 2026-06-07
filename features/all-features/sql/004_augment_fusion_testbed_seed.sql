-- Testbed seed content for the Live Items augment fusion prototype.
-- This is NPC fusion, not true nested augments.

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
VALUES
(1, 'Items:LiveItemLoading', 'true', 'Enable live DB item fallback'),
(1, 'Items:LiveItemMinID', '150000', 'First DB-backed live item ID'),
(1, 'Items:LiveItemMaxID', '199999', 'Last DB-backed live item ID; 950000-999998 is reserved for dynamic item client display IDs'),
(1, 'Items:LiveItemPollIntervalSeconds', '1', 'Live item DB poll interval')
ON DUPLICATE KEY UPDATE
`rule_value` = VALUES(`rule_value`),
`notes` = VALUES(`notes`);

DELETE FROM spawn2 WHERE id = 900905 OR spawngroupID = 900905;
DELETE FROM spawnentry WHERE spawngroupID = 900905 OR npcID = 900905;
DELETE FROM spawngroup WHERE id = 900905;
DELETE FROM npc_types WHERE id = 900905;
DELETE FROM items WHERE id BETWEEN 199211 AND 199220;

REPLACE INTO items (
  id, minstatus, Name, lore, comment, itemclass, itemtype, classes, races,
  norent, nodrop, size, weight, magic, loregroup, maxcharges, price, sellrate,
  questitemflag, updated, icon, stackable, stacksize, slots,
  astr, asta, adex, aagi, aint, awis, acha,
  hp, mana, endur, ac, mr, fr, cr, dr, pr, svcorruption,
  attack, accuracy, avoidance, regen, manaregen, enduranceregen,
  healamt, spelldmg, clairvoyance,
  augslot1type, augslot1visible, augslot2type, augslot2visible, augtype, augrestrict
) VALUES
(199211, 0, 'Augment Catalyst', 'A catalytic shard used for augment fusion.', 'Seeded by Live Items augment fusion test SQL', 0, 54, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 646, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0),
(199212, 0, 'Fusion Test Tide Shard', 'A prototype augment shard.', 'Seeded by Live Items augment fusion test SQL', 0, 54, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 646, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 25, 0, 0, 0, 0, 5, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0),
(199213, 0, 'Fusion Test Stone Shard', 'A prototype augment shard.', 'Seeded by Live Items augment fusion test SQL', 0, 54, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 646, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 10, 0, 10, 5, 5, 0, 0, 5, 0, 0, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 7, 0),
(199214, 0, 'Fusion Test Focus Shard', 'A prototype augment shard.', 'Seeded by Live Items augment fusion test SQL', 0, 54, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 646, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 10, 0, 0, 0, 0, 0, 0, 5, 0, 0, 3, 0, 1, 0, 0, 3, 3, 1, 0, 0, 0, 0, 7, 0),
(199220, 0, 'Fusion Test Socket Sash', 'A test item with visible type 7 augment slots.', 'Seeded by Live Items augment fusion test SQL', 0, 10, 65535, 65535, 255, 0, 3, 8, 1, 0, -1, 0, 1, 1, NOW(), 538, 0, 0, 131072, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 1, 7, 1, 0, 0);

REPLACE INTO npc_types (
  id, name, lastname, level, race, `class`, bodytype, hp, gender, texture,
  size, runspeed, findable, trackable, isquest, show_name, special_abilities,
  skip_global_loot
) VALUES
(900905, 'Nalyx_Augmentweaver', 'Augment Fusion Prototype', 70, 1, 1, 1, 100000, 0, 0, 6.0, 1.25, 1, 1, 1, 1, '19,1', 1);

REPLACE INTO spawngroup (id, name, spawn_limit) VALUES
(900905, 'live_items_nalyx_augmentweaver', 1);

REPLACE INTO spawnentry (spawngroupID, npcID, chance) VALUES
(900905, 900905, 100);

REPLACE INTO spawn2 (
  id, spawngroupID, zone, version, x, y, z, heading, respawntime, variance
) VALUES
(900905, 900905, 'tutorialb', 0, -24.0, -80.0, 25.0, 128.0, 300, 0);
