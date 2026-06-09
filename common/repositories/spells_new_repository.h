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

#include "common/repositories/base/base_spells_new_repository.h"

#include "common/database.h"
#include "common/strings.h"

class SpellsNewRepository: public BaseSpellsNewRepository {
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
     * SpellsNewRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * SpellsNewRepository::GetWhereNeverExpires()
     * SpellsNewRepository::GetWhereXAndY()
     * SpellsNewRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	// Custom extended repository methods here
	static std::vector<std::string> GetSpellFileLines(Database& db)
	{
		std::vector<std::string> lines;

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT_WS('^', {}) FROM {} ORDER BY {} ASC",
				ColumnsRaw(),
				TableName(),
				PrimaryKey()
			)
		);

		for (auto row : results) {
			lines.emplace_back(row[0]);
		}

		return lines;
	}

	static std::vector<std::string> GetMulticlassSpellFileLines(Database& db)
	{
		std::vector<std::string> lines;

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT_WS('^', {}) FROM {} ORDER BY {} ASC",
				ColumnsRaw(),
				TableName(),
				PrimaryKey()
			)
		);

		for (auto row : results) {
			auto columns = Strings::Split(row[0] ? row[0] : "", '^');

			if (columns.size() > 98 && (columns[98] == "14" || columns[98] == "38")) {
				columns[98] = "6";
			}

			if (columns.size() > 149) {
				columns[149] = "1";
			}

			if (columns.size() > 168 && columns[168] == "-1") {
				int valid_class_count = 0;
				int valid_class_id = -1;

				for (int column = 104; column <= 119 && column < static_cast<int>(columns.size()); ++column) {
					const auto spell_level = Strings::ToInt(columns[column]);
					if (spell_level > 0 && spell_level <= 70) {
						++valid_class_count;
						valid_class_id = column - 103;
					}
				}

				if (valid_class_count == 1 && valid_class_id > 0 && columns.size() > 167) {
					columns[167] = std::to_string(Strings::ToInt(columns[167]) + (20 * valid_class_id));
				}
			}

			lines.emplace_back(Strings::Join(columns, "^"));
		}

		return lines;
	}
};
