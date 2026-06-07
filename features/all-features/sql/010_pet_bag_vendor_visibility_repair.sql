-- Make Syncrosatchel vendor stock visible to all classes.
-- The bag item rows still carry their own class restrictions.

START TRANSACTION;

UPDATE `merchantlist`
SET `classes_required` = 65535
WHERE `merchantid` IN (900907, 151022)
  AND `item` IN (899980, 899981, 899983, 899984, 899985, 899986, 899987, 899988, 17725, 17726, 17727, 900000);

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 14);

COMMIT;
