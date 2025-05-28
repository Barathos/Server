#include "database_update.h"

std::vector<ManifestEntry> manifest_entries_custom = {
	ManifestEntry{
		.version = 1,
		.description = "2025_05_16_new_database_check_test",
		.check = "SHOW TABLES LIKE 'new_table'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `new_table`  (
  `id` int NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`id`)
);
)",
		.content_schema_update = false,
	},

	// Content schema tables
	ManifestEntry{
		.version = 2,
		.description = "2025_05_16_thj_waypoints_content_tables",
		.check = "SHOW TABLES LIKE 'thj_waypoints_categories'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE thj_waypoints_categories (
    id INT PRIMARY KEY,
    name VARCHAR(32) NOT NULL UNIQUE
);

CREATE TABLE thj_waypoints (
    id INT AUTO_INCREMENT PRIMARY KEY,
    shortname VARCHAR(32) UNIQUE NOT NULL,
    long_name VARCHAR(64) NOT NULL,
    category INT NOT NULL,
    x FLOAT NOT NULL,
    y FLOAT NOT NULL,
    z FLOAT NOT NULL,
    heading FLOAT NOT NULL,
    INDEX idx_thj_waypoints_shortname (shortname),
    INDEX idx_thj_waypoints_category (category)
);

CREATE TABLE thj_waypoints_default (
    id INT AUTO_INCREMENT PRIMARY KEY,
    waypoint_id INT NOT NULL,
    race_id INT NOT NULL DEFAULT 0,
    class_mask INT UNSIGNED NOT NULL DEFAULT 65535,
    min_level INT NOT NULL DEFAULT 1,
    max_level INT NOT NULL DEFAULT 255,
    UNIQUE KEY unique_default_waypoint (waypoint_id, race_id, class_mask, min_level),
    INDEX idx_thj_waypoints_default_race (race_id),
    INDEX idx_thj_waypoints_default_class (class_mask),
    INDEX idx_thj_waypoints_default_level (min_level, max_level)
);
)",
		.content_schema_update = true,
	},

	// PEQ schema tables
	ManifestEntry{
		.version = 3,
		.description = "2025_05_16_thj_waypoints_peq_tables",
		.check = "SHOW TABLES LIKE 'thj_waypoints_character'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE thj_waypoints_character (
    id INT AUTO_INCREMENT PRIMARY KEY,
    character_id BIGINT UNSIGNED NOT NULL,
    waypoint_id INT NOT NULL,
    unlock_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_character_waypoint (character_id, waypoint_id),
    INDEX idx_thj_waypoints_character_id (character_id)
);

CREATE TABLE thj_waypoints_account (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account_id BIGINT UNSIGNED NOT NULL,
    waypoint_id INT NOT NULL,
    unlock_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_account_waypoint (account_id, waypoint_id),
    INDEX idx_thj_waypoints_account_id (account_id)
);
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version = 4,
		.description = "2025_05_20_thj_zone_npc_update_range",
		.check = "SHOW COLUMNS FROM `zone` LIKE 'npc_update_range'",
		.condition = "empty",
		.match = "",
		.sql = R"(
ALTER TABLE `zone` ADD COLUMN `npc_update_range` int(11) NOT NULL DEFAULT 600 AFTER `npc_max_aggro_dist`;
)",
		.content_schema_update = true,
	},

	ManifestEntry{
		.version = 5,
		.description = "2025_05_20_thj_zone_max_movement_range",
		.check = "SHOW COLUMNS FROM `zone` LIKE 'max_movement_update_range'",
		.condition = "empty",
		.match = "",
		.sql = R"(
ALTER TABLE `zone` ADD COLUMN `max_movement_update_range` int(11) NOT NULL DEFAULT 600 AFTER `npc_update_range`;
)",
		.content_schema_update = true,
	},

	ManifestEntry{
		.version = 6,
		.description = "2025_05_20_thj_global_buffs",
		.check = "SHOW TABLES LIKE 'global_buffs'",
		.condition = "empty",
		.match = "",
		.sql = R"(
create table global_buffs(spell_id int(11) primary key, duration int(11));
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version = 7,
		.description = "2025_05_20_thj_account_kill_counts",
		.check = "SHOW TABLES LIKE 'account_kill_counts'",
		.condition = "empty",
		.match = "",
		.sql = R"(
create table account_kill_counts(account_id int(11) primary key, race_id int(11), count int(11));
)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version = 8,
		.description = "2025_05_20_thj_character_pet_class_id",
		.check = "SHOW COLUMNS FROM `character_pet_name` LIKE 'class_id'",
		.condition = "empty",
		.match = "",
		.sql = R"(
	ALTER TABLE `character_pet_name`
		DROP PRIMARY KEY,
		ADD COLUMN `class_id` TINYINT(11) NOT NULL DEFAULT -1,
		ADD PRIMARY KEY (`character_id`, `class_id`);
	)",
		.content_schema_update = false,
	},

	ManifestEntry{
		.version = 9,
		.description = "2025_05_20_thj_account_alt_currency",
		.check = "SHOW TABLES LIKE 'account_alt_currency'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `account_alt_currency` (
  `account_id` int(10) unsigned NOT NULL,
  `currency_id` int(10) unsigned NOT NULL,
  `amount` int(10) unsigned NOT NULL,
  PRIMARY KEY (`account_id`, `currency_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

INSERT INTO account_alt_currency (account_id, currency_id, amount)
SELECT
  cd.account_id,
  cac.currency_id,
  SUM(cac.amount) AS amount
FROM character_alt_currency cac
JOIN character_data cd ON cd.id = cac.char_id
GROUP BY cd.account_id, cac.currency_id
ON DUPLICATE KEY UPDATE
  amount = VALUES(amount);
)",
		.content_schema_update = false,
	},

	// New familiar_names table
	ManifestEntry{
		.version = 10,
		.description = "2025_05_20_familiar_names_table",
		.check = "SHOW TABLES LIKE 'familiar_names'",
		.condition = "empty",
		.match = "",
		.sql = R"(
	CREATE TABLE `familiar_names` (
	  `spell_id` int(10) NOT NULL,
	  `name_list` text NOT NULL,
	  `size_mod` int(10) NOT NULL DEFAULT -1,
	  PRIMARY KEY (`spell_id`)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
	)",
		.content_schema_update = true,
	},

	// Alter character_pet_name to change class_id from TINYINT to INT
	ManifestEntry{
		.version = 11,
		.description = "2025_05_20_modify_character_pet_name_class_id",
		.check = "SHOW COLUMNS FROM `character_pet_name` WHERE `Field` = 'class_id' AND `Type` LIKE 'tinyint%'",
		.condition = "not_empty",
		.match = "",
		.sql = R"(
			ALTER TABLE `character_pet_name`
			MODIFY COLUMN `class_id` INT NOT NULL DEFAULT 0
			)",
		.content_schema_update = false,
	},

	// Add character_aa_disabled table
	ManifestEntry{
		.version = 12,
		.description = "2025_05_20_thj_character_aa_disabled",
		.check = "SHOW TABLES LIKE 'character_aa_disabled'",
		.condition = "empty",
		.match = "",
		.sql = R"(
	CREATE TABLE `character_aa_disabled` (
	`aa_id` int(10) NOT NULL,
	`character_id` int(10) NOT NULL,
	`disabled` tinyint(4) NOT NULL,
	PRIMARY KEY (`aa_id`,`character_id`),
	KEY `idx_character_id` (`character_id`)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
	)",
		.content_schema_update = false,
	},

	// Add table for pet command states
	ManifestEntry{
		.version = 13,
		.description = "2025_05_26_character_pet_command_states_table",
		.check = "SHOW TABLES LIKE 'character_pet_command_states'",
		.condition = "empty",
		.match = "",
		.sql = R"(
	CREATE TABLE `character_pet_command_states` (
	`character_id` int(10) NOT NULL,
	`pet_class` tinyint(4) NOT NULL,
	`command_id` tinyint(4) NOT NULL,
	`command_state` tinyint(4) NOT NULL,
	PRIMARY KEY (`character_id`,`pet_class`,`command_id`),
	KEY `idx_char_petclass` (`character_id`,`pet_class`)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
	)",
		.content_schema_update = false,
	},

	// Add character_dynamic_aa_timers table
	ManifestEntry{
		.version = 14,
		.description = "2025_05_27_character_dynamic_aa_timers_table",
		.check = "SHOW TABLES LIKE 'character_dynamic_aa_timers'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `character_dynamic_aa_timers` (
  `character_id` int(10) NOT NULL,
  `aa_id` int(10) NOT NULL,
  `timer_id` int(10) NOT NULL,
  PRIMARY KEY (`character_id`,`aa_id`),
  UNIQUE KEY `character_id` (`character_id`,`aa_id`,`timer_id`),
  KEY `aa_id` (`aa_id`),
  KEY `timer_id` (`timer_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)",
		.content_schema_update = false,
	},

	// Used for testing
	//	ManifestEntry{
	//		.version = 9229,
	//		.description = "new_database_check_test",
	//		.check = "SHOW TABLES LIKE 'new_table'",
	//		.condition = "empty",
	//		.match = "",
	//		.sql = R"(
	// CREATE TABLE `new_table`  (
	//  `id` int NOT NULL AUTO_INCREMENT,
	//  PRIMARY KEY (`id`)
	//);
	// CREATE TABLE `new_table1`  (
	//  `id` int NOT NULL AUTO_INCREMENT,
	//  PRIMARY KEY (`id`)
	//);
	// CREATE TABLE `new_table2`  (
	//  `id` int NOT NULL AUTO_INCREMENT,
	//  PRIMARY KEY (`id`)
	//);
	// CREATE TABLE `new_table3`  (
	//  `id` int NOT NULL AUTO_INCREMENT,
	//  PRIMARY KEY (`id`)
	//);
	//)",
	//	}
};

// see struct definitions for what each field does
// struct ManifestEntry {
// 	int         version{};     // database version of the migration
// 	std::string description{}; // description of the migration ex: "add_new_table" or "add_index_to_table"
// 	std::string check{};       // query that checks against the condition
// 	std::string condition{};   // condition or "match_type" - Possible values [contains|match|missing|empty|not_empty]
// 	std::string match{};       // match field that is not always used, but works in conjunction with "condition" values [missing|match|contains]
// 	std::string sql{};         // the SQL DDL that gets ran when the condition is true
// };
