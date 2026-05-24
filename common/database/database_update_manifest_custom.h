/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "common/database/database_update.h"

#include <vector>

std::vector<ManifestEntry> manifest_entries_custom = {
	ManifestEntry{
		.version = 1,
		.description = "2026_05_14_source_backed_autoloot",
		.check = "SHOW TABLES LIKE 'custom_autoloot_settings'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `custom_autoloot_settings` (
  `character_id` INT UNSIGNED NOT NULL,
  `enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `filter_mode` ENUM('both','include','exclude') NOT NULL DEFAULT 'both',
  `debug_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `log_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`)
);

CREATE TABLE `custom_autoloot_filters` (
  `character_id` INT UNSIGNED NOT NULL,
  `item_id` INT UNSIGNED NOT NULL,
  `filter_mode` ENUM('include','exclude') NOT NULL,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `item_id`, `filter_mode`),
  KEY `idx_item_id` (`item_id`)
);

CREATE TABLE `custom_autoloot_autosell_exclusions` (
  `character_id` INT UNSIGNED NOT NULL,
  `item_id` INT UNSIGNED NOT NULL,
  `created_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `item_id`),
  KEY `idx_item_id` (`item_id`)
);

CREATE TABLE `custom_autoloot_group_settings` (
  `group_id` INT UNSIGNED NOT NULL,
  `loot_mode` ENUM('none','solo','master','robin','killer','assigned') NOT NULL DEFAULT 'solo',
  `assigned_character_id` INT UNSIGNED NOT NULL DEFAULT 0,
  `round_robin_index` INT UNSIGNED NOT NULL DEFAULT 0,
  `need_greed_enabled` TINYINT(1) NOT NULL DEFAULT 0,
  `updated_at` INT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`group_id`)
);

CREATE TABLE `custom_autoloot_audit` (
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
);
)",
		.content_schema_update = false,
	},
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
