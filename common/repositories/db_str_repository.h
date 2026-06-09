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

#include "common/repositories/base/base_db_str_repository.h"

#include "common/database.h"
#include "common/strings.h"

#include <unordered_map>
#include <vector>

class DbStrRepository: public BaseDbStrRepository {
public:

    /**
     * This file was auto generated and can be modified and extended upon
     *
     * Base repository methods are automatically
     * generated in the "base" version of this repository. The base repository
     * is immutable and to be left untouched, while methods in this class
     * are used as extension methods for more specific persistence-layer
     * accessors or mutators.
     *
     * Base Methods (Subject to be expanded upon in time)
     *
     * Note: Not all tables are designed appropriately to fit functionality with all base methods
     *
     * InsertOne
     * UpdateOne
     * DeleteOne
     * FindOne
     * GetWhere(std::string where_filter)
     * DeleteWhere(std::string where_filter)
     * InsertMany
     * All
     *
     * Example custom methods in a repository
     *
     * DbStrRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * DbStrRepository::GetWhereNeverExpires()
     * DbStrRepository::GetWhereXAndY()
     * DbStrRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	// Custom extended repository methods here
	static std::vector<std::string> GetDBStrFileLines(Database& db)
	{
		std::vector<std::string> lines;

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT(CONCAT_WS('^', {}), '^0') FROM {} ORDER BY `id`, `type` ASC",
				ColumnsRaw(),
				TableName()
			)
		);

		for (auto row : results) {
			lines.emplace_back(row[0]);
		}

		return lines;
	}

	static std::vector<std::string> GetMulticlassDBStrFileLines(Database& db)
	{
		std::vector<std::string> lines;

		const std::vector<std::pair<int, std::string>> class_map = {
			{1, "WAR"}, {2, "CLR"}, {4, "PAL"}, {8, "RNG"}, {16, "SHD"}, {32, "DRU"},
			{64, "MNK"}, {128, "BRD"}, {256, "ROG"}, {512, "SHM"}, {1024, "NEC"},
			{2048, "WIZ"}, {4096, "MAG"}, {8192, "ENC"}, {16384, "BST"}, {32768, "BER"}
		};

		std::unordered_map<std::string, int> desc_sid_to_classes;
		auto class_results = db.QueryDatabase(
			"SELECT `aa_ranks`.`desc_sid`, `aa_ability`.`classes` "
			"FROM `aa_ability` "
			"JOIN `aa_ranks` ON `aa_ability`.`first_rank_id` = `aa_ranks`.`id`"
		);
		for (auto row : class_results) {
			if (row[0]) {
				desc_sid_to_classes[row[0]] = row[1] ? Strings::ToInt(row[1]) : 0;
			}
		}

		struct ActivationInfo {
			int recast_time = 0;
			int spell_id = 0;
			int ability_id = 0;
		};

		std::unordered_map<std::string, ActivationInfo> desc_sid_to_activation;
		auto activation_results = db.QueryDatabase(
			"SELECT `aa_ranks`.`desc_sid`, `aa_ranks`.`recast_time`, `aa_ranks`.`spell`, `aa_ability`.`id` "
			"FROM `aa_ability` "
			"JOIN `aa_ranks` ON `aa_ability`.`first_rank_id` = `aa_ranks`.`id`"
		);
		for (auto row : activation_results) {
			if (row[0]) {
				desc_sid_to_activation[row[0]] = {
					row[1] ? Strings::ToInt(row[1]) : 0,
					row[2] ? Strings::ToInt(row[2]) : 0,
					row[3] ? Strings::ToInt(row[3]) : 0
				};
			}
		}

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT(CONCAT_WS('^', {}), '^0') FROM {} ORDER BY `id`, `type` ASC",
				ColumnsRaw(),
				TableName()
			)
		);

		for (auto row : results) {
			auto columns = Strings::Split(row[0] ? row[0] : "", '^');
			if (columns.size() > 2 && columns[1] == "4") {
				const auto class_match = desc_sid_to_classes.find(columns[0]);
				if (class_match != desc_sid_to_classes.end()) {
					std::string activation_info;
					const auto activation_match = desc_sid_to_activation.find(columns[0]);
					if (activation_match != desc_sid_to_activation.end() && activation_match->second.ability_id > 0) {
						const auto &activation = activation_match->second;
						activation_info = activation.recast_time > 0 && activation.spell_id > -1 ?
							fmt::format(" [/alt activate {}]", activation.ability_id) :
							fmt::format(" [/alt toggle {}]", activation.ability_id);
					}

					std::vector<std::string> class_tags;
					for (const auto &[class_bit, class_name] : class_map) {
						if (class_match->second & class_bit) {
							class_tags.emplace_back(class_name);
						}
					}

					const auto tag = class_tags.empty() ? "(ALL)" : fmt::format("({})", Strings::Join(class_tags, " "));
					columns[2] = fmt::format("{}{}<br>{}", tag, activation_info, columns[2]);
				}
			}

			lines.emplace_back(Strings::Join(columns, "^"));
		}

		return lines;
	}
};
