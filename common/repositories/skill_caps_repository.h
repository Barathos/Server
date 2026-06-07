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

#include "common/repositories/base/base_skill_caps_repository.h"

#include "common/database.h"
#include "common/strings.h"

class SkillCapsRepository: public BaseSkillCapsRepository {
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
     * SkillCapsRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * SkillCapsRepository::GetWhereNeverExpires()
     * SkillCapsRepository::GetWhereXAndY()
     * SkillCapsRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	// Custom extended repository methods here
	static std::vector<std::string> GetSkillCapFileLines(Database& db)
	{
		std::vector<std::string> lines;

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT_WS('^', `class_id`, `skill_id`, `level`, `cap`, `class_`) FROM {} ORDER BY `class_id`, `skill_id`, `level` ASC",
				TableName()
			)
		);

		for (auto row : results) {
			lines.emplace_back(row[0]);
		}

		return lines;
	}

	static std::vector<std::string> GetMulticlassSkillCapFileLines(Database& db)
	{
		std::vector<std::string> lines;

		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT CONCAT_WS('^', class_ids.class_id, caps.skill_id, caps.level, caps.cap, 0) "
				"FROM ("
				"SELECT 1 AS class_id UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4 "
				"UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 "
				"UNION ALL SELECT 9 UNION ALL SELECT 10 UNION ALL SELECT 11 UNION ALL SELECT 12 "
				"UNION ALL SELECT 13 UNION ALL SELECT 14 UNION ALL SELECT 15 UNION ALL SELECT 16"
				") AS class_ids "
				"JOIN ("
				"SELECT skill_id, level, MAX(cap) AS cap "
				"FROM {} "
				"GROUP BY skill_id, level"
				") AS caps "
				"WHERE caps.cap > 0 "
				"ORDER BY class_ids.class_id, caps.skill_id, caps.level ASC",
				TableName()
			)
		);

		for (auto row : results) {
			lines.emplace_back(row[0]);
		}

		return lines;
	}
};
