-- Live-range HPFIX equipment for validating the high-HP native overlay.
-- These rows stay inside Items:LiveItemMinID/MaxID so they can be summoned
-- through the live item path without rebuilding item shared memory.

REPLACE INTO `items` (
  `id`, `minstatus`, `Name`, `lore`, `comment`, `itemclass`, `itemtype`, `classes`, `races`,
  `norent`, `nodrop`, `size`, `weight`, `magic`, `loregroup`, `maxcharges`, `price`, `sellrate`,
  `questitemflag`, `updated`, `icon`, `idfile`, `stackable`, `stacksize`, `slots`,
  `hp`, `mana`, `endur`, `ac`
) VALUES
(199990, 0, 'HPFIX Live 12M Charm', 'HPFIX Live 12M Charm', 'Seeded by HPFIX live item validation SQL', 0, 52, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 1123, 'IT63', 0, 1, 1, 12000000, 0, 0, 25),
(199991, 0, 'HPFIX Live 25M Breastplate', 'HPFIX Live 25M Breastplate', 'Seeded by HPFIX live item validation SQL', 0, 10, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 538, 'IT63', 0, 1, 131072, 25000000, 0, 0, 100),
(199992, 0, 'HPFIX Live 50M Girdle', 'HPFIX Live 50M Girdle', 'Seeded by HPFIX live item validation SQL', 0, 10, 65535, 65535, 255, 0, 1, 1, 1, 0, -1, 0, 1, 1, NOW(), 549, 'IT63', 0, 1, 1048576, 50000000, 0, 0, 50);
