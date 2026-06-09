/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/types.h"

#include <map>
#include <string>
#include <vector>

namespace EQ::AdvancedLoot
{
	enum class VoteChoice {
		Unset,
		Need,
		Greed,
		Pass
	};

	enum class LootFilterDecision {
		Unset,
		AlwaysNeed,
		AlwaysGreed,
		Never
	};

	struct VoteResolution {
		bool ready = false;
		std::vector<uint32> need;
		std::vector<uint32> greed;
		std::vector<uint32> winner_pool;
	};

	std::string VoteChoiceState(VoteChoice choice);
	std::string VoteChoiceLabel(VoteChoice choice);

	bool IsValidFilterDecision(const std::string &decision);
	LootFilterDecision ParseFilterDecision(const std::string &decision);
	std::string FilterDecisionKey(LootFilterDecision decision);
	std::string FilterDecisionLabel(LootFilterDecision decision);
	bool FilterDecisionAutoLootsPersonal(LootFilterDecision decision);
	VoteChoice FilterDecisionVoteChoice(LootFilterDecision decision);

	VoteResolution ResolveVotes(
		const std::vector<uint32> &eligible_character_ids,
		const std::map<uint32, VoteChoice> &votes,
		bool timeout
	);
}
