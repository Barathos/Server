-- HPFIX high-HP equipment used to validate the native client HP display overlay.
-- These items intentionally push item HP beyond 10 million total HP.

DELETE FROM items WHERE id IN (990001, 990002, 990003);

CREATE TEMPORARY TABLE hpfix_item_seed LIKE items;

INSERT INTO hpfix_item_seed SELECT * FROM items WHERE id = 1001 LIMIT 1;
UPDATE hpfix_item_seed
SET
	id = 990001,
	Name = 'HPFIX 12M Charm',
	lore = 'HPFIX 12M Charm',
	hp = 12000000,
	ac = 25,
	mana = 0,
	endur = 0,
	slots = 1,
	classes = 65535,
	races = 65535,
	icon = 1054,
	weight = 1,
	size = 1,
	minstatus = 0;
INSERT INTO items SELECT * FROM hpfix_item_seed;

TRUNCATE TABLE hpfix_item_seed;
INSERT INTO hpfix_item_seed SELECT * FROM items WHERE id = 1001 LIMIT 1;
UPDATE hpfix_item_seed
SET
	id = 990002,
	Name = 'HPFIX 25M Breastplate',
	lore = 'HPFIX 25M Breastplate',
	hp = 25000000,
	ac = 100,
	mana = 0,
	endur = 0,
	slots = 131072,
	classes = 65535,
	races = 65535,
	icon = 625,
	weight = 1,
	size = 1,
	minstatus = 0;
INSERT INTO items SELECT * FROM hpfix_item_seed;

TRUNCATE TABLE hpfix_item_seed;
INSERT INTO hpfix_item_seed SELECT * FROM items WHERE id = 1001 LIMIT 1;
UPDATE hpfix_item_seed
SET
	id = 990003,
	Name = 'HPFIX 50M Girdle',
	lore = 'HPFIX 50M Girdle',
	hp = 50000000,
	ac = 50,
	mana = 0,
	endur = 0,
	slots = 1048576,
	classes = 65535,
	races = 65535,
	icon = 549,
	weight = 1,
	size = 1,
	minstatus = 0;
INSERT INTO items SELECT * FROM hpfix_item_seed;

DROP TEMPORARY TABLE hpfix_item_seed;
