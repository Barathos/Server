ALTER TABLE `custom_advloot_settings`
  MODIFY `auto_show_loot_window` TINYINT(1) NOT NULL DEFAULT 0;

UPDATE `custom_advloot_settings`
SET `auto_show_loot_window` = 0;

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 21);
