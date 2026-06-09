ALTER TABLE `custom_advloot_settings`
  MODIFY `auto_show_loot_window` TINYINT(1) NOT NULL DEFAULT 1;

ALTER TABLE `custom_advloot_settings`
  MODIFY `show_new_items_only` TINYINT(1) NOT NULL DEFAULT 1;

UPDATE `custom_advloot_settings`
SET `auto_show_loot_window` = 1,
    `show_new_items_only` = 1;

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 22);
