-- Source-backed native AutoLoot schema.
-- This standalone SQL mirrors the AutoLoot-owned objects from the custom migration.

CREATE TABLE IF NOT EXISTS `custom_autoloot_settings` (
  `character_id` INT UNSIGNED NOT NULL,
  `enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `filter_mode` ENUM('both','include','exclude') NOT NULL DEFAULT 'both',
  `debug_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `log_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_autoloot_filters` (
  `character_id` INT UNSIGNED NOT NULL,
  `item_id` INT UNSIGNED NOT NULL,
  `filter_mode` ENUM('include','exclude') NOT NULL,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `item_id`, `filter_mode`),
  KEY `idx_item_id` (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_autoloot_autosell_exclusions` (
  `character_id` INT UNSIGNED NOT NULL,
  `item_id` INT UNSIGNED NOT NULL,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `item_id`),
  KEY `idx_item_id` (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_autoloot_group_settings` (
  `group_id` INT UNSIGNED NOT NULL,
  `loot_mode` ENUM('none','solo','master','robin','killer','assigned') NOT NULL DEFAULT 'solo',
  `assigned_character_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `round_robin_index` INT UNSIGNED NOT NULL DEFAULT 0,
  `need_greed_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_autoloot_audit` (
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
