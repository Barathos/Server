/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "advanced_loot.h"

#include "common/strings.h"

std::string EQ::AdvancedLoot::VoteChoiceState(VoteChoice choice)
{
	switch (choice) {
	case VoteChoice::Need:
		return "need";
	case VoteChoice::Greed:
		return "greed";
	case VoteChoice::Pass:
		return "no";
	default:
		return "waiting";
	}
}

std::string EQ::AdvancedLoot::VoteChoiceLabel(VoteChoice choice)
{
	switch (choice) {
	case VoteChoice::Need:
		return "Need";
	case VoteChoice::Greed:
		return "Greed";
	case VoteChoice::Pass:
		return "No";
	default:
		return "Waiting";
	}
}

bool EQ::AdvancedLoot::IsValidFilterDecision(const std::string &decision)
{
	return Strings::EqualFold(decision, "unset") ||
		Strings::EqualFold(decision, "always_need") ||
		Strings::EqualFold(decision, "always_greed") ||
		Strings::EqualFold(decision, "never") ||
		Strings::EqualFold(decision, "an") ||
		Strings::EqualFold(decision, "ag") ||
		Strings::EqualFold(decision, "nv") ||
		Strings::EqualFold(decision, "need") ||
		Strings::EqualFold(decision, "greed") ||
		Strings::EqualFold(decision, "no");
}

EQ::AdvancedLoot::LootFilterDecision EQ::AdvancedLoot::ParseFilterDecision(const std::string &decision)
{
	if (Strings::EqualFold(decision, "always_need") || Strings::EqualFold(decision, "an") || Strings::EqualFold(decision, "need")) {
		return LootFilterDecision::AlwaysNeed;
	}

	if (Strings::EqualFold(decision, "always_greed") || Strings::EqualFold(decision, "ag") || Strings::EqualFold(decision, "greed")) {
		return LootFilterDecision::AlwaysGreed;
	}

	if (Strings::EqualFold(decision, "never") || Strings::EqualFold(decision, "nv") || Strings::EqualFold(decision, "no")) {
		return LootFilterDecision::Never;
	}

	return LootFilterDecision::Unset;
}

std::string EQ::AdvancedLoot::FilterDecisionKey(LootFilterDecision decision)
{
	switch (decision) {
	case LootFilterDecision::AlwaysNeed:
		return "always_need";
	case LootFilterDecision::AlwaysGreed:
		return "always_greed";
	case LootFilterDecision::Never:
		return "never";
	default:
		return "unset";
	}
}

std::string EQ::AdvancedLoot::FilterDecisionLabel(LootFilterDecision decision)
{
	switch (decision) {
	case LootFilterDecision::AlwaysNeed:
		return "AN";
	case LootFilterDecision::AlwaysGreed:
		return "AG";
	case LootFilterDecision::Never:
		return "NV";
	default:
		return "-";
	}
}

bool EQ::AdvancedLoot::FilterDecisionAutoLootsPersonal(LootFilterDecision decision)
{
	return decision == LootFilterDecision::AlwaysNeed || decision == LootFilterDecision::AlwaysGreed;
}

EQ::AdvancedLoot::VoteChoice EQ::AdvancedLoot::FilterDecisionVoteChoice(LootFilterDecision decision)
{
	switch (decision) {
	case LootFilterDecision::AlwaysNeed:
		return VoteChoice::Need;
	case LootFilterDecision::AlwaysGreed:
		return VoteChoice::Greed;
	case LootFilterDecision::Never:
		return VoteChoice::Pass;
	default:
		return VoteChoice::Unset;
	}
}

std::string EQ::AdvancedLoot::VoteChoiceKey(VoteChoice choice)
{
	switch (choice) {
	case VoteChoice::Need:
		return "need";
	case VoteChoice::Greed:
		return "greed";
	case VoteChoice::Pass:
		return "pass";
	default:
		return "off";
	}
}

EQ::AdvancedLoot::VoteChoice EQ::AdvancedLoot::ParseSessionDefaultChoice(const std::string &value)
{
	if (Strings::EqualFold(value, "need")) {
		return VoteChoice::Need;
	}

	if (Strings::EqualFold(value, "greed")) {
		return VoteChoice::Greed;
	}

	if (Strings::EqualFold(value, "pass") || Strings::EqualFold(value, "no") || Strings::EqualFold(value, "leave")) {
		return VoteChoice::Pass;
	}

	// "off", "unset", and anything unrecognized clear the default.
	return VoteChoice::Unset;
}

void EQ::AdvancedLoot::SessionDefaultStore::Set(uint32 character_id, VoteChoice choice)
{
	if (choice == VoteChoice::Unset) {
		entries.erase(character_id);
		return;
	}

	entries[character_id] = choice;
}

EQ::AdvancedLoot::VoteChoice EQ::AdvancedLoot::SessionDefaultStore::Resolve(uint32 character_id) const
{
	auto it = entries.find(character_id);
	return it == entries.end() ? VoteChoice::Unset : it->second;
}

void EQ::AdvancedLoot::SessionDefaultStore::Clear(uint32 character_id)
{
	entries.erase(character_id);
}

EQ::AdvancedLoot::VoteResolution EQ::AdvancedLoot::ResolveVotes(
	const std::vector<uint32> &eligible_character_ids,
	const std::map<uint32, VoteChoice> &votes,
	bool timeout
)
{
	VoteResolution result;

	if (!timeout) {
		for (const auto character_id : eligible_character_ids) {
			auto vote_iter = votes.find(character_id);
			if (vote_iter == votes.end() || vote_iter->second == VoteChoice::Unset) {
				return result;
			}
		}
	}

	result.ready = true;
	for (const auto character_id : eligible_character_ids) {
		auto vote_iter = votes.find(character_id);
		if (vote_iter == votes.end()) {
			continue;
		}

		if (vote_iter->second == VoteChoice::Need) {
			result.need.push_back(character_id);
		}
		else if (vote_iter->second == VoteChoice::Greed) {
			result.greed.push_back(character_id);
		}
	}

	result.winner_pool = !result.need.empty() ? result.need : result.greed;
	return result;
}
