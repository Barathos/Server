/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/advanced_loot.h"
#include "cppunit/cpptest.h"

#include <map>
#include <string>
#include <vector>

class AdvancedLootLogicTest : public Test::Suite {
public:
	AdvancedLootLogicTest()
	{
		TEST_ADD(AdvancedLootLogicTest::FilterAliasesParseToCanonicalDecisions);
		TEST_ADD(AdvancedLootLogicTest::FilterKeysAndLabelsRemainStable);
		TEST_ADD(AdvancedLootLogicTest::VoteStatesAndLabelsRemainStable);
		TEST_ADD(AdvancedLootLogicTest::NonTimeoutVotesWaitForEveryEligiblePlayer);
		TEST_ADD(AdvancedLootLogicTest::TimeoutVotesIgnoreMissingAndUnsetVotes);
		TEST_ADD(AdvancedLootLogicTest::NeedPoolBeatsGreedAndPassNeverWins);
	}

private:
	using LootFilterDecision = EQ::AdvancedLoot::LootFilterDecision;
	using VoteChoice = EQ::AdvancedLoot::VoteChoice;

	void AssertDecision(const std::string &value, LootFilterDecision expected)
	{
		TEST_ASSERT(EQ::AdvancedLoot::IsValidFilterDecision(value));
		TEST_ASSERT(EQ::AdvancedLoot::ParseFilterDecision(value) == expected);
	}

	void FilterAliasesParseToCanonicalDecisions()
	{
		AssertDecision("need", LootFilterDecision::AlwaysNeed);
		AssertDecision("an", LootFilterDecision::AlwaysNeed);
		AssertDecision("always_need", LootFilterDecision::AlwaysNeed);
		AssertDecision("greed", LootFilterDecision::AlwaysGreed);
		AssertDecision("ag", LootFilterDecision::AlwaysGreed);
		AssertDecision("always_greed", LootFilterDecision::AlwaysGreed);
		AssertDecision("never", LootFilterDecision::Never);
		AssertDecision("nv", LootFilterDecision::Never);
		AssertDecision("no", LootFilterDecision::Never);
		AssertDecision("unset", LootFilterDecision::Unset);

		TEST_ASSERT(!EQ::AdvancedLoot::IsValidFilterDecision("vendor"));
		TEST_ASSERT(EQ::AdvancedLoot::ParseFilterDecision("vendor") == LootFilterDecision::Unset);
	}

	void FilterKeysAndLabelsRemainStable()
	{
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionKey(LootFilterDecision::AlwaysNeed) == "always_need");
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionKey(LootFilterDecision::AlwaysGreed) == "always_greed");
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionKey(LootFilterDecision::Never) == "never");
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionKey(LootFilterDecision::Unset) == "unset");

		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionLabel(LootFilterDecision::AlwaysNeed) == "AN");
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionLabel(LootFilterDecision::AlwaysGreed) == "AG");
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionLabel(LootFilterDecision::Never) == "NV");
		TEST_ASSERT(EQ::AdvancedLoot::FilterDecisionLabel(LootFilterDecision::Unset) == "-");
	}

	void VoteStatesAndLabelsRemainStable()
	{
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceState(VoteChoice::Need) == "need");
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceState(VoteChoice::Greed) == "greed");
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceState(VoteChoice::Pass) == "no");
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceState(VoteChoice::Unset) == "waiting");

		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceLabel(VoteChoice::Need) == "Need");
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceLabel(VoteChoice::Greed) == "Greed");
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceLabel(VoteChoice::Pass) == "No");
		TEST_ASSERT(EQ::AdvancedLoot::VoteChoiceLabel(VoteChoice::Unset) == "Waiting");
	}

	void NonTimeoutVotesWaitForEveryEligiblePlayer()
	{
		const std::vector<uint32> eligible = {101, 102, 103};
		const std::map<uint32, VoteChoice> missing_vote = {
			{101, VoteChoice::Need},
			{102, VoteChoice::Greed}
		};
		const auto missing = EQ::AdvancedLoot::ResolveVotes(eligible, missing_vote, false);
		TEST_ASSERT(!missing.ready);

		const std::map<uint32, VoteChoice> unset_vote = {
			{101, VoteChoice::Need},
			{102, VoteChoice::Greed},
			{103, VoteChoice::Unset}
		};
		const auto unset = EQ::AdvancedLoot::ResolveVotes(eligible, unset_vote, false);
		TEST_ASSERT(!unset.ready);

		const std::map<uint32, VoteChoice> complete_votes = {
			{101, VoteChoice::Need},
			{102, VoteChoice::Greed},
			{103, VoteChoice::Pass}
		};
		const auto complete = EQ::AdvancedLoot::ResolveVotes(eligible, complete_votes, false);
		TEST_ASSERT(complete.ready);
		TEST_ASSERT_EQUALS(static_cast<size_t>(1), complete.need.size());
		TEST_ASSERT_EQUALS(static_cast<uint32>(101), complete.need[0]);
	}

	void TimeoutVotesIgnoreMissingAndUnsetVotes()
	{
		const std::vector<uint32> eligible = {101, 102, 103, 104};
		const std::map<uint32, VoteChoice> votes = {
			{101, VoteChoice::Unset},
			{102, VoteChoice::Greed}
		};

		const auto resolution = EQ::AdvancedLoot::ResolveVotes(eligible, votes, true);
		TEST_ASSERT(resolution.ready);
		TEST_ASSERT(resolution.need.empty());
		TEST_ASSERT_EQUALS(static_cast<size_t>(1), resolution.greed.size());
		TEST_ASSERT_EQUALS(static_cast<uint32>(102), resolution.greed[0]);
		TEST_ASSERT_EQUALS(static_cast<size_t>(1), resolution.winner_pool.size());
		TEST_ASSERT_EQUALS(static_cast<uint32>(102), resolution.winner_pool[0]);
	}

	void NeedPoolBeatsGreedAndPassNeverWins()
	{
		const std::vector<uint32> eligible = {101, 102, 103, 104};
		const std::map<uint32, VoteChoice> mixed_votes = {
			{101, VoteChoice::Greed},
			{102, VoteChoice::Need},
			{103, VoteChoice::Pass},
			{104, VoteChoice::Need}
		};

		const auto mixed = EQ::AdvancedLoot::ResolveVotes(eligible, mixed_votes, false);
		TEST_ASSERT(mixed.ready);
		TEST_ASSERT_EQUALS(static_cast<size_t>(2), mixed.need.size());
		TEST_ASSERT_EQUALS(static_cast<size_t>(1), mixed.greed.size());
		TEST_ASSERT_EQUALS(static_cast<size_t>(2), mixed.winner_pool.size());
		TEST_ASSERT_EQUALS(static_cast<uint32>(102), mixed.winner_pool[0]);
		TEST_ASSERT_EQUALS(static_cast<uint32>(104), mixed.winner_pool[1]);

		const std::map<uint32, VoteChoice> all_pass = {
			{101, VoteChoice::Pass},
			{102, VoteChoice::Pass},
			{103, VoteChoice::Pass},
			{104, VoteChoice::Pass}
		};
		const auto pass = EQ::AdvancedLoot::ResolveVotes(eligible, all_pass, false);
		TEST_ASSERT(pass.ready);
		TEST_ASSERT(pass.need.empty());
		TEST_ASSERT(pass.greed.empty());
		TEST_ASSERT(pass.winner_pool.empty());

		const auto no_votes_timeout = EQ::AdvancedLoot::ResolveVotes(eligible, {}, true);
		TEST_ASSERT(no_votes_timeout.ready);
		TEST_ASSERT(no_votes_timeout.winner_pool.empty());
	}
};
