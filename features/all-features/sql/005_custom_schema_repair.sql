-- Idempotent repair for public-test databases whose custom_version was already
-- advanced before all custom feature tables existed.

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

CREATE TABLE IF NOT EXISTS `custom_achievement_rewards` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_type` VARCHAR(32) NOT NULL DEFAULT '',
  `reward_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `amount` INT UNSIGNED NOT NULL DEFAULT 1,
  `auto_claim` TINYINT(1) NOT NULL DEFAULT 0,
  `tier` VARCHAR(32) NOT NULL DEFAULT '',
  `preview_text` VARCHAR(255) NOT NULL DEFAULT '',
  `data_text` TEXT NULL,
  `chance` SMALLINT UNSIGNED NOT NULL DEFAULT 10000,
  `sort_order` INT NOT NULL DEFAULT 0,
  `enabled` TINYINT(1) NOT NULL DEFAULT 1,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_achievement_sort` (`achievement_id`, `enabled`, `sort_order`, `id`),
  KEY `idx_reward_type` (`reward_type`, `reward_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_character_achievement_rewards` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_definition_id` BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `reward_type` VARCHAR(32) NOT NULL DEFAULT '',
  `reward_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `amount` INT UNSIGNED NOT NULL DEFAULT 1,
  `auto_claim` TINYINT(1) NOT NULL DEFAULT 0,
  `tier` VARCHAR(32) NOT NULL DEFAULT '',
  `preview_text` VARCHAR(255) NOT NULL DEFAULT '',
  `data_text` TEXT NULL,
  `status` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `completion_count` INT UNSIGNED NOT NULL DEFAULT 1,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `claimed_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `result_text` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_character_reward` (`character_id`, `achievement_id`, `reward_definition_id`, `reward_type`, `reward_id`, `completion_count`),
  KEY `idx_character_status` (`character_id`, `status`, `created_at`, `id`),
  KEY `idx_achievement` (`achievement_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_achievement_live_item_requests` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `reward_queue_id` BIGINT UNSIGNED NOT NULL,
  `level_band` INT UNSIGNED NOT NULL DEFAULT 0,
  `tier` VARCHAR(32) NOT NULL DEFAULT '',
  `item_slot` INT UNSIGNED NOT NULL DEFAULT 0,
  `theme` VARCHAR(64) NOT NULL DEFAULT '',
  `status` VARCHAR(32) NOT NULL DEFAULT 'pending',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `result_text` VARCHAR(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_reward_queue` (`reward_queue_id`),
  KEY `idx_character_status` (`character_id`, `status`, `created_at`),
  KEY `idx_achievement` (`achievement_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_account_achievement_unlocks` (
  `account_id` INT UNSIGNED NOT NULL,
  `achievement_id` INT UNSIGNED NOT NULL,
  `character_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `unlock_type` VARCHAR(32) NOT NULL DEFAULT '',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`account_id`, `achievement_id`, `unlock_type`),
  KEY `idx_achievement` (`achievement_id`),
  KEY `idx_character` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

UPDATE `db_version` SET `custom_version` = GREATEST(`custom_version`, 9);
