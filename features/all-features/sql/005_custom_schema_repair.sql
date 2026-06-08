-- Idempotent repair for public-test databases whose custom_version was already
-- advanced before all custom feature tables existed. Legacy custom_autoloot_*
-- tables are intentionally not recreated; version 19 drops any leftovers and
-- creates the current custom_advloot_* schema.

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
