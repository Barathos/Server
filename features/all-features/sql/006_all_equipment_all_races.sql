-- Make every equippable item usable by all playable races.
-- EQEmu item race masks use 65535 for all playable races.

UPDATE `items`
SET `races` = 65535
WHERE `slots` <> 0
  AND `races` <> 65535;

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 10);
