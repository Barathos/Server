DROP TABLE IF EXISTS `custom_autoloot_audit`;
DROP TABLE IF EXISTS `custom_autoloot_group_settings`;
DROP TABLE IF EXISTS `custom_autoloot_autosell_exclusions`;
DROP TABLE IF EXISTS `custom_autoloot_filters`;
DROP TABLE IF EXISTS `custom_autoloot_settings`;

CREATE TABLE IF NOT EXISTS `custom_advloot_settings` (
  `character_id` INT UNSIGNED NOT NULL,
  `use_advanced_looting` TINYINT(1) NOT NULL DEFAULT 1,
  `apply_filters` TINYINT(1) NOT NULL DEFAULT 1,
  `auto_split_coin` TINYINT(1) NOT NULL DEFAULT 1,
  `confirm_remove_filter` TINYINT(1) NOT NULL DEFAULT 1,
  `auto_remove_looted_lore` TINYINT(1) NOT NULL DEFAULT 1,
  `auto_show_loot_window` TINYINT(1) NOT NULL DEFAULT 1,
  `show_new_items_only` TINYINT(1) NOT NULL DEFAULT 0,
  `auto_loot_all` TINYINT(1) NOT NULL DEFAULT 0,
  `master_looter_candidate` TINYINT(1) NOT NULL DEFAULT 1,
  `debug_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `log_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_advloot_filters` (
  `character_id` INT UNSIGNED NOT NULL,
  `item_id` INT UNSIGNED NOT NULL,
  `decision` ENUM('unset','always_need','always_greed','never') NOT NULL DEFAULT 'unset',
  `auto_ask_roll` TINYINT(1) NOT NULL DEFAULT 0,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `item_id`),
  KEY `idx_item_id` (`item_id`),
  KEY `idx_decision` (`decision`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_advloot_audit` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `action` VARCHAR(64) NOT NULL DEFAULT '',
  `item_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `quantity` INT UNSIGNED NOT NULL DEFAULT 0,
  `detail` VARCHAR(255) NOT NULL DEFAULT '',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_character_created` (`character_id`, `created_at`),
  KEY `idx_item_id` (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 19);
