-- Enhanced personal tribute choices for the all-features bundle.
-- These use the stock Tribute window, stock tribute selection packets, and
-- server-side bonus hooks gated by CustomFeatures:EnhancedTributeEnabled.

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
VALUES
(1, 'CustomFeatures:EnhancedTributeEnabled', 'true', 'Enable custom high-impact Tribute-window choices and server-side enhanced tribute bonuses')
ON DUPLICATE KEY UPDATE
`rule_value` = VALUES(`rule_value`),
`notes` = VALUES(`notes`);

DELETE FROM `tribute_levels` WHERE `tribute_id` BETWEEN 900100 AND 900105;
DELETE FROM `tributes` WHERE `id` BETWEEN 900100 AND 900105 AND `isguild` = 0;
DELETE FROM `items` WHERE `id` BETWEEN 199300 AND 199329;

REPLACE INTO `tributes` (`id`, `unknown`, `name`, `descr`, `isguild`) VALUES
(900100, 5, 'Blood Engine', 'Turns favor into violence.<br>Benefit -<br>Melee crit chance, crit damage, accuracy, flurry, extra attacks, double melee rounds, and melee lifetap scale hard each tier.', 0),
(900101, 5, 'Spell Furnace', 'Overclocks every caster and healer toolkit.<br>Benefit -<br>Spell crits, DoT crits, heal crits, spell damage, heal amount, mana regeneration, and channeling all increase each tier.', 0),
(900102, 5, 'Titan''s Aegis', 'Spends tribute like a shield wall made of coin.<br>Benefit -<br>Max HP, flat HP, AC, regeneration, mitigation, avoidance, shield blocks, stun resistance, delay death, and damage shield improve each tier.', 0),
(900103, 5, 'Packlord''s Pact', 'Your companion gets a war budget.<br>Benefit -<br>Pet max HP, attack, flurry, critical hits, avoidance, mitigation, and rampage chance improve each tier.', 0),
(900104, 5, 'Wayfarer''s Momentum', 'For players who want the world itself to move faster.<br>Benefit -<br>Run speed, run speed cap, bonus forage, salvage chance, faction gains, packrat, forage access, and tradeskill safety improve each tier.', 0),
(900105, 5, 'Executioner''s Omen', 'Makes low-health enemies feel the bill coming due.<br>Benefit -<br>Finishing Blow chance, threshold, and damage scale aggressively, with extra all-skill damage and undead punishment each tier.', 0);

INSERT INTO `tribute_levels` (`tribute_id`, `level`, `cost`, `item_id`) VALUES
(900100, 1, 250, 199300),
(900100, 20, 750, 199301),
(900100, 40, 2000, 199302),
(900100, 60, 5000, 199303),
(900100, 70, 12000, 199304),
(900101, 1, 250, 199305),
(900101, 20, 750, 199306),
(900101, 40, 2000, 199307),
(900101, 60, 5000, 199308),
(900101, 70, 12000, 199309),
(900102, 1, 250, 199310),
(900102, 20, 750, 199311),
(900102, 40, 2000, 199312),
(900102, 60, 5000, 199313),
(900102, 70, 12000, 199314),
(900103, 1, 250, 199315),
(900103, 20, 750, 199316),
(900103, 40, 2000, 199317),
(900103, 60, 5000, 199318),
(900103, 70, 12000, 199319),
(900104, 1, 250, 199320),
(900104, 20, 750, 199321),
(900104, 40, 2000, 199322),
(900104, 60, 5000, 199323),
(900104, 70, 12000, 199324),
(900105, 1, 250, 199325),
(900105, 20, 750, 199326),
(900105, 40, 2000, 199327),
(900105, 60, 5000, 199328),
(900105, 70, 12000, 199329);

REPLACE INTO `items` (
  `id`, `minstatus`, `Name`, `lore`, `comment`, `itemclass`, `itemtype`, `classes`, `races`,
  `norent`, `nodrop`, `size`, `weight`, `magic`, `loregroup`, `maxcharges`, `price`, `sellrate`,
  `questitemflag`, `benefitflag`, `updated`, `icon`, `idfile`, `stackable`, `stacksize`, `slots`,
  `hp`, `mana`, `endur`, `ac`, `attack`, `accuracy`, `avoidance`, `regen`, `manaregen`, `enduranceregen`,
  `healamt`, `spelldmg`, `clairvoyance`, `haste`, `shielding`, `spellshield`, `strikethrough`, `stunresist`,
  `damageshield`, `heroic_str`, `heroic_sta`, `heroic_dex`, `heroic_agi`, `heroic_int`, `heroic_wis`, `heroic_cha`
) VALUES
(199300, 0, 'Blood Engine I Benefit', 'Blood Engine I Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1185, 'IT63', 0, 1, 0, 250, 0, 250, 25, 75, 25, 0, 0, 0, 5, 0, 0, 0, 40, 0, 0, 5, 0, 0, 5, 5, 5, 0, 0, 0, 0),
(199301, 0, 'Blood Engine II Benefit', 'Blood Engine II Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1185, 'IT63', 0, 1, 0, 500, 0, 500, 50, 150, 50, 0, 0, 0, 10, 0, 0, 0, 50, 0, 0, 10, 0, 0, 10, 10, 10, 0, 0, 0, 0),
(199302, 0, 'Blood Engine III Benefit', 'Blood Engine III Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1185, 'IT63', 0, 1, 0, 750, 0, 750, 75, 225, 75, 0, 0, 0, 15, 0, 0, 0, 60, 0, 0, 15, 0, 0, 15, 15, 15, 0, 0, 0, 0),
(199303, 0, 'Blood Engine IV Benefit', 'Blood Engine IV Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1185, 'IT63', 0, 1, 0, 1000, 0, 1000, 100, 300, 100, 0, 0, 0, 20, 0, 0, 0, 70, 0, 0, 20, 0, 0, 20, 20, 20, 0, 0, 0, 0),
(199304, 0, 'Blood Engine V Benefit', 'Blood Engine V Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1185, 'IT63', 0, 1, 0, 1250, 0, 1250, 125, 375, 125, 0, 0, 0, 25, 0, 0, 0, 80, 0, 0, 25, 0, 0, 25, 25, 25, 0, 0, 0, 0),
(199305, 0, 'Spell Furnace I Benefit', 'Spell Furnace I Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1186, 'IT63', 0, 1, 0, 0, 500, 0, 10, 0, 0, 0, 0, 10, 0, 50, 50, 20, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5),
(199306, 0, 'Spell Furnace II Benefit', 'Spell Furnace II Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1186, 'IT63', 0, 1, 0, 0, 1000, 0, 20, 0, 0, 0, 0, 20, 0, 100, 100, 40, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 10, 10, 10),
(199307, 0, 'Spell Furnace III Benefit', 'Spell Furnace III Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1186, 'IT63', 0, 1, 0, 0, 1500, 0, 30, 0, 0, 0, 0, 30, 0, 150, 150, 60, 0, 0, 15, 0, 0, 0, 0, 0, 0, 0, 15, 15, 15),
(199308, 0, 'Spell Furnace IV Benefit', 'Spell Furnace IV Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1186, 'IT63', 0, 1, 0, 0, 2000, 0, 40, 0, 0, 0, 0, 40, 0, 200, 200, 80, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 20, 20, 20),
(199309, 0, 'Spell Furnace V Benefit', 'Spell Furnace V Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1186, 'IT63', 0, 1, 0, 0, 2500, 0, 50, 0, 0, 0, 0, 50, 0, 250, 250, 100, 0, 0, 25, 0, 0, 0, 0, 0, 0, 0, 25, 25, 25),
(199310, 0, 'Titan''s Aegis I Benefit', 'Titan''s Aegis I Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1187, 'IT63', 0, 1, 0, 1000, 0, 250, 125, 0, 0, 10, 20, 0, 5, 0, 0, 0, 0, 5, 0, 0, 5, 25, 0, 8, 0, 8, 0, 0, 0),
(199311, 0, 'Titan''s Aegis II Benefit', 'Titan''s Aegis II Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1187, 'IT63', 0, 1, 0, 2000, 0, 500, 250, 0, 0, 20, 40, 0, 10, 0, 0, 0, 0, 10, 0, 0, 10, 50, 0, 16, 0, 16, 0, 0, 0),
(199312, 0, 'Titan''s Aegis III Benefit', 'Titan''s Aegis III Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1187, 'IT63', 0, 1, 0, 3000, 0, 750, 375, 0, 0, 30, 60, 0, 15, 0, 0, 0, 0, 15, 0, 0, 15, 75, 0, 24, 0, 24, 0, 0, 0),
(199313, 0, 'Titan''s Aegis IV Benefit', 'Titan''s Aegis IV Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1187, 'IT63', 0, 1, 0, 4000, 0, 1000, 500, 0, 0, 40, 80, 0, 20, 0, 0, 0, 0, 20, 0, 0, 20, 100, 0, 32, 0, 32, 0, 0, 0),
(199314, 0, 'Titan''s Aegis V Benefit', 'Titan''s Aegis V Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1187, 'IT63', 0, 1, 0, 5000, 0, 1250, 625, 0, 0, 50, 100, 0, 25, 0, 0, 0, 0, 25, 0, 0, 25, 125, 0, 40, 0, 40, 0, 0, 0),
(199315, 0, 'Packlord''s Pact I Benefit', 'Packlord''s Pact I Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1188, 'IT63', 0, 1, 0, 500, 250, 250, 50, 50, 10, 0, 10, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4),
(199316, 0, 'Packlord''s Pact II Benefit', 'Packlord''s Pact II Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1188, 'IT63', 0, 1, 0, 1000, 500, 500, 100, 100, 20, 0, 20, 10, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8),
(199317, 0, 'Packlord''s Pact III Benefit', 'Packlord''s Pact III Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1188, 'IT63', 0, 1, 0, 1500, 750, 750, 150, 150, 30, 0, 30, 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 12, 12, 12, 12, 12, 12),
(199318, 0, 'Packlord''s Pact IV Benefit', 'Packlord''s Pact IV Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1188, 'IT63', 0, 1, 0, 2000, 1000, 1000, 200, 200, 40, 0, 40, 20, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 16, 16, 16, 16, 16, 16),
(199319, 0, 'Packlord''s Pact V Benefit', 'Packlord''s Pact V Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1188, 'IT63', 0, 1, 0, 2500, 1250, 1250, 250, 250, 50, 0, 50, 25, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 20, 20, 20, 20, 20, 20),
(199320, 0, 'Wayfarer''s Momentum I Benefit', 'Wayfarer''s Momentum I Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1189, 'IT63', 0, 1, 0, 250, 250, 250, 25, 25, 10, 10, 5, 5, 5, 0, 0, 0, 10, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 8),
(199321, 0, 'Wayfarer''s Momentum II Benefit', 'Wayfarer''s Momentum II Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1189, 'IT63', 0, 1, 0, 500, 500, 500, 50, 50, 20, 20, 10, 10, 10, 0, 0, 0, 20, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 16),
(199322, 0, 'Wayfarer''s Momentum III Benefit', 'Wayfarer''s Momentum III Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1189, 'IT63', 0, 1, 0, 750, 750, 750, 75, 75, 30, 30, 15, 15, 15, 0, 0, 0, 30, 0, 0, 0, 0, 0, 9, 9, 9, 9, 9, 9, 24),
(199323, 0, 'Wayfarer''s Momentum IV Benefit', 'Wayfarer''s Momentum IV Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1189, 'IT63', 0, 1, 0, 1000, 1000, 1000, 100, 100, 40, 40, 20, 20, 20, 0, 0, 0, 40, 0, 0, 0, 0, 0, 12, 12, 12, 12, 12, 12, 32),
(199324, 0, 'Wayfarer''s Momentum V Benefit', 'Wayfarer''s Momentum V Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1189, 'IT63', 0, 1, 0, 1250, 1250, 1250, 125, 125, 50, 50, 25, 25, 25, 0, 0, 0, 50, 0, 0, 0, 0, 0, 15, 15, 15, 15, 15, 15, 40),
(199325, 0, 'Executioner''s Omen I Benefit', 'Executioner''s Omen I Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1190, 'IT63', 0, 1, 0, 250, 0, 500, 25, 100, 25, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 10, 0, 0, 8, 0, 8, 0, 0, 0, 0),
(199326, 0, 'Executioner''s Omen II Benefit', 'Executioner''s Omen II Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1190, 'IT63', 0, 1, 0, 500, 0, 1000, 50, 200, 50, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 20, 0, 0, 16, 0, 16, 0, 0, 0, 0),
(199327, 0, 'Executioner''s Omen III Benefit', 'Executioner''s Omen III Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1190, 'IT63', 0, 1, 0, 750, 0, 1500, 75, 300, 75, 0, 0, 0, 15, 0, 0, 0, 0, 0, 0, 30, 0, 0, 24, 0, 24, 0, 0, 0, 0),
(199328, 0, 'Executioner''s Omen IV Benefit', 'Executioner''s Omen IV Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1190, 'IT63', 0, 1, 0, 1000, 0, 2000, 100, 400, 100, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 40, 0, 0, 32, 0, 32, 0, 0, 0, 0),
(199329, 0, 'Executioner''s Omen V Benefit', 'Executioner''s Omen V Benefit', 'Seeded by enhanced tribute SQL', 0, 0, 65535, 65535, 255, 0, 1, 0, 1, -1, -1, 0, 1, 1, 1, NOW(), 1190, 'IT63', 0, 1, 0, 1250, 0, 2500, 125, 500, 125, 0, 0, 0, 25, 0, 0, 0, 0, 0, 0, 50, 0, 0, 40, 0, 40, 0, 0, 0, 0);
