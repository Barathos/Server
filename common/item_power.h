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

#include "common/types.h"

#include <string>
#include <vector>

class Database;

namespace EQ
{
	struct ItemData;

	namespace ItemPower
	{
		constexpr uint16 ScoreVersion = 1;

		enum class Role : uint8 {
			Tank = 0,
			Melee,
			Caster,
			Healer,
			Hybrid
		};

		struct ScoreComponent {
			std::string component;
			int32       score = 0;
			std::string details;
		};

		struct ScoreResult {
			uint32 item_id       = 0;
			uint16 item_level    = 1;
			uint32 item_score    = 0;
			uint32 tank_score    = 0;
			uint32 melee_score   = 0;
			uint32 caster_score  = 0;
			uint32 healer_score  = 0;
			uint32 hybrid_score  = 0;
			uint16 score_version = ScoreVersion;

			Role best_role = Role::Tank;
			std::string source = "computed";

			int32 base_stat_score = 0;
			int32 defense_score   = 0;
			int32 offense_score   = 0;
			int32 sustain_score   = 0;
			int32 weapon_score    = 0;
			int32 effect_score    = 0;

			double slot_budget = 1.0;
			std::string slot_budget_name = "Default";

			std::vector<ScoreComponent> breakdown;
			std::vector<std::string> warnings;
		};

		struct StoredScore {
			uint32 item_id       = 0;
			uint16 item_level    = 1;
			uint32 item_score    = 0;
			uint32 tank_score    = 0;
			uint32 melee_score   = 0;
			uint32 caster_score  = 0;
			uint32 healer_score  = 0;
			uint32 hybrid_score  = 0;
			uint16 score_version = ScoreVersion;
			std::string source;
			std::string updated_at;
		};

		bool EnsureSchema(Database &db);
		bool SchemaReady(Database &db);

		ScoreResult Calculate(const ItemData &item, bool include_breakdown = true);
		bool ApplyOverrides(Database &db, ScoreResult &score);
		bool SaveScore(
			Database &db,
			const ScoreResult &score,
			std::string *error_message = nullptr,
			bool save_breakdown = true,
			bool use_transaction = true
		);
		bool TryGetStoredScore(Database &db, uint32 item_id, StoredScore &score);

		bool SetLevelOverride(Database &db, uint32 item_id, uint16 item_level);
		bool SetMultiplierOverride(Database &db, uint32 item_id, float multiplier);
		bool SetFlatBonusOverride(Database &db, uint32 item_id, int32 flat_bonus);
		bool SetOverrideNotes(Database &db, uint32 item_id, const std::string &notes);
		bool ClearOverride(Database &db, uint32 item_id);

		const char *RoleName(Role role);
		const char *RoleKey(Role role);
		Role BestRoleFromScores(const StoredScore &score);

		std::string BuildTransportMessage(const ItemData &item, const ScoreResult &score);
		std::string BuildTransportMessage(const ItemData &item, const StoredScore &score);
		bool TryBuildTransportMessage(Database &db, const ItemData &item, std::string &message, bool calculate_if_missing);
		bool TryBuildStoredTransportMessage(Database &db, const ItemData &item, std::string &message);
	}
}
