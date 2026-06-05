-- Syncrosatchel pet bags.
-- Runtime defaults live in common/ruletypes.h; this persists the public testbed value
-- and seeds the class-specific containers used by Client::IsValidPetBagForClass.

START TRANSACTION;

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
VALUES
(1, 'CustomFeatures:PetBagsEnabled', 'true', 'Enable Syncrosatchel pet bags for summoned and charmed pets')
ON DUPLICATE KEY UPDATE
`rule_value` = VALUES(`rule_value`),
`notes` = VALUES(`notes`);

REPLACE INTO `items` (
  `id`, `minstatus`, `Name`, `lore`, `comment`, `itemclass`, `itemtype`,
  `classes`, `races`, `norent`, `nodrop`, `size`, `weight`, `magic`,
  `loregroup`, `maxcharges`, `price`, `sellrate`, `questitemflag`,
  `updated`, `icon`, `idfile`, `stackable`, `stacksize`, `slots`,
  `bagsize`, `bagslots`, `bagtype`, `bagwr`, `charmfile`, `charmfileid`,
  `color`, `summonedflag`
) VALUES
(899980, 0, 'Shadowknight''s Syncrosatchel', 'Your Shadowknight pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 16, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4483, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899981, 0, 'Druid''s Syncrosatchel', 'Your Druid pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 32, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4481, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899983, 0, 'Bard''s Syncrosatchel', 'Your Bard pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 128, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4464, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899984, 0, 'Shaman''s Syncrosatchel', 'Your Shaman pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 512, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4491, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899985, 0, 'Necromancer''s Syncrosatchel', 'Your Necromancer pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 1024, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4469, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899986, 0, 'Magician''s Syncrosatchel', 'Your Magician pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 4096, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4470, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899987, 0, 'Enchanter''s Syncrosatchel', 'Your Enchanter pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 8192, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4488, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(899988, 0, 'Beastlord''s Syncrosatchel', 'Your Beastlord pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 16384, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4489, 'IT63', 0, 1, 0, 5, 4, 0, 100, '', '', 4278190080, 1),
(17725, 0, 'Beastlord''s Greater Syncrosatchel', 'Your Beastlord pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 16384, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4489, 'IT63', 0, 1, 0, 5, 8, 0, 100, '', '', 4278190080, 1),
(17726, 0, 'Enchanter''s Greater Syncrosatchel', 'Your Enchanter pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 8192, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4488, 'IT63', 0, 1, 0, 5, 8, 0, 100, '', '', 4278190080, 1),
(17727, 0, 'Necromancer''s Greater Syncrosatchel', 'Your Necromancer pet will equip items placed within this container.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 1024, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 4469, 'IT63', 0, 1, 0, 5, 8, 0, 100, '', '', 4278190080, 1),
(900000, 0, 'Summoned: Dimensional Armory', 'Allows access to an armory of equipment at a mere thought.', 'Seeded by Syncrosatchel pet bag SQL', 1, 11, 4096, 65535, 1, 0, 1, 0, 1, -1, -1, 0, 1, 0, NOW(), 6560, 'IT63', 0, 1, 0, 4, 15, 0, 100, '', '', 4278190080, 1);

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 11);

COMMIT;
