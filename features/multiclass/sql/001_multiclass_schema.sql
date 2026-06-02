CREATE TABLE `custom_multiclass_profiles` (
  `character_id` INT UNSIGNED NOT NULL,
  `class_slot_1` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `class_slot_2` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `class_slot_3` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `trio_name` VARCHAR(96) NOT NULL DEFAULT '',
  `resonance_key` VARCHAR(64) NOT NULL DEFAULT '',
  `multiple_pets_enabled` TINYINT(1) NOT NULL DEFAULT 1,
  `locked` TINYINT(1) NOT NULL DEFAULT 0,
  `reweaves_available` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`),
  KEY `idx_slot_1` (`class_slot_1`),
  KEY `idx_slot_2` (`class_slot_2`),
  KEY `idx_slot_3` (`class_slot_3`),
  KEY `idx_resonance` (`resonance_key`)
);

CREATE TABLE `custom_multiclass_profile_audit` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `character_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `class_slot` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `class_id` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `action` VARCHAR(64) NOT NULL DEFAULT '',
  `detail` VARCHAR(255) NOT NULL DEFAULT '',
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_character_created` (`character_id`, `created_at`),
  KEY `idx_class` (`class_id`)
);

CREATE TABLE `custom_multiclass_pet_state` (
  `character_id` INT UNSIGNED NOT NULL,
  `pet_slot` TINYINT UNSIGNED NOT NULL,
  `spell_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `pet_name` VARCHAR(64) NOT NULL DEFAULT '',
  `pet_order` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `held` TINYINT(1) NOT NULL DEFAULT 0,
  `gheld` TINYINT(1) NOT NULL DEFAULT 0,
  `nocast` TINYINT(1) NOT NULL DEFAULT 0,
  `focused` TINYINT(1) NOT NULL DEFAULT 0,
  `pet_stop` TINYINT(1) NOT NULL DEFAULT 0,
  `pet_regroup` TINYINT(1) NOT NULL DEFAULT 0,
  `guard_x` FLOAT NOT NULL DEFAULT 0,
  `guard_y` FLOAT NOT NULL DEFAULT 0,
  `guard_z` FLOAT NOT NULL DEFAULT 0,
  `guard_heading` FLOAT NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `pet_slot`),
  KEY `idx_character_spell` (`character_id`, `spell_id`)
);

CREATE TABLE `custom_multiclass_bard_melody` (
  `character_id` INT UNSIGNED NOT NULL,
  `slot_id` TINYINT UNSIGNED NOT NULL,
  `spell_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `slot_id`)
);
