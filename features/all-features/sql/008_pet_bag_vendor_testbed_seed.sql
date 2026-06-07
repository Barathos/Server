-- Tutorialb free pet bag vendor for public testing.
-- Places Satra Syncrosatchel behind Nalyx Augmentweaver and stocks every
-- Syncrosatchel pet bag seeded by custom migration 11.

START TRANSACTION;

DELETE FROM `merchantlist` WHERE `merchantid` = 900907;
DELETE FROM `spawn2` WHERE `id` = 900907 OR `spawngroupID` = 900907;
DELETE FROM `spawnentry` WHERE `spawngroupID` = 900907 OR `npcID` = 900907;
DELETE FROM `spawngroup` WHERE `id` = 900907;
DELETE FROM `npc_types` WHERE `id` = 900907;

REPLACE INTO `npc_types` (
  `id`, `name`, `lastname`, `level`, `race`, `class`, `bodytype`, `hp`,
  `gender`, `texture`, `merchant_id`, `size`, `runspeed`, `findable`,
  `trackable`, `npc_aggro`, `isquest`, `show_name`, `special_abilities`,
  `skip_global_loot`, `keeps_sold_items`
) VALUES
(900907, 'Satra_Syncrosatchel', 'Free Pet Bag Vendor', 70, 1, 41, 1, 100000, 1, 0, 900907, 6.0, 1.25, 1, 1, 0, 0, 1, '19,1', 1, 0);

REPLACE INTO `spawngroup` (`id`, `name`, `spawn_limit`) VALUES
(900907, 'all_features_satra_syncrosatchel', 1);

REPLACE INTO `spawnentry` (`spawngroupID`, `npcID`, `chance`) VALUES
(900907, 900907, 100);

REPLACE INTO `spawn2` (
  `id`, `spawngroupID`, `zone`, `version`, `x`, `y`, `z`, `heading`,
  `respawntime`, `variance`
) VALUES
(900907, 900907, 'tutorialb', 0, -24.0, -88.0, 25.0, 128.0, 300, 0);

INSERT INTO `merchantlist` (
  `merchantid`, `slot`, `item`, `faction_required`, `level_required`,
  `min_status`, `max_status`, `alt_currency_cost`, `classes_required`,
  `probability`
) VALUES
(900907, 1, 899980, -100, 0, 0, 255, 0, 65535, 100),
(900907, 2, 899981, -100, 0, 0, 255, 0, 65535, 100),
(900907, 3, 899983, -100, 0, 0, 255, 0, 65535, 100),
(900907, 4, 899984, -100, 0, 0, 255, 0, 65535, 100),
(900907, 5, 899985, -100, 0, 0, 255, 0, 65535, 100),
(900907, 6, 899986, -100, 0, 0, 255, 0, 65535, 100),
(900907, 7, 899987, -100, 0, 0, 255, 0, 65535, 100),
(900907, 8, 899988, -100, 0, 0, 255, 0, 65535, 100),
(900907, 9, 17725, -100, 0, 0, 255, 0, 65535, 100),
(900907, 10, 17726, -100, 0, 0, 255, 0, 65535, 100),
(900907, 11, 17727, -100, 0, 0, 255, 0, 65535, 100),
(900907, 12, 900000, -100, 0, 0, 255, 0, 65535, 100)
ON DUPLICATE KEY UPDATE
`slot` = VALUES(`slot`),
`faction_required` = VALUES(`faction_required`),
`level_required` = VALUES(`level_required`),
`min_status` = VALUES(`min_status`),
`max_status` = VALUES(`max_status`),
`alt_currency_cost` = VALUES(`alt_currency_cost`),
`classes_required` = VALUES(`classes_required`),
`probability` = VALUES(`probability`);

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 12);

COMMIT;
