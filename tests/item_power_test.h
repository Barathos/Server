/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/classes.h"
#include "common/emu_constants.h"
#include "common/item_data.h"
#include "common/item_instance.h"
#include "common/item_power.h"
#include "common/strings.h"
#include "cppunit/cpptest.h"

class ItemPowerTest : public Test::Suite {
public:
	ItemPowerTest()
	{
		TEST_ADD(ItemPowerTest::NonEquippableItemsAreNotScored);
		TEST_ADD(ItemPowerTest::StableRoleFixturesKeepGoldenScores);
		TEST_ADD(ItemPowerTest::ProgressionWeightedScoreRanksHigherLevelHammerAboveSimpleDagger);
		TEST_ADD(ItemPowerTest::AugmentedInstanceScoreIncludesAugments);
		TEST_ADD(ItemPowerTest::BreakdownOnlyBuildsWhenRequested);
		TEST_ADD(ItemPowerTest::RoleNamesKeysAndTransportAreStable);
	}

private:
	EQ::ItemData BuildItem(uint32 id, const char *name, uint8 item_type, uint32 slots, uint8 req_level = 0) const
	{
		EQ::ItemData item{};
		item.ID = id;
		item.ItemClass = EQ::item::ItemClassCommon;
		item.ItemType = item_type;
		item.Slots = slots;
		item.Classes = Class::ALL_CLASSES_BITMASK;
		item.Races = 0xFFFFFFFF;
		item.ReqLevel = req_level;
		item.NoDrop = 255;
		item.NoRent = 255;
		strn0cpy(item.Name, name, sizeof(item.Name));
		strn0cpy(item.Lore, name, sizeof(item.Lore));
		strn0cpy(item.IDFile, "IT10", sizeof(item.IDFile));
		return item;
	}

	EQ::ItemData BuildTankChest() const
	{
		auto item = BuildItem(910001, "Fixture Tank Chest", EQ::item::ItemTypeArmor, 1u << EQ::invslot::slotChest, 30);
		item.HP = 200;
		item.AC = 40;
		item.ASta = 20;
		item.AAgi = 10;
		item.Avoidance = 10;
		item.Shielding = 5;
		return item;
	}

	EQ::ItemData BuildMeleeSword() const
	{
		auto item = BuildItem(910002, "Fixture Melee Sword", EQ::item::ItemType1HSlash, 1u << EQ::invslot::slotPrimary, 20);
		item.Damage = 30;
		item.Delay = 24;
		item.AStr = 20;
		item.ADex = 20;
		item.Attack = 60;
		item.Accuracy = 20;
		item.Haste = 25;
		return item;
	}

	EQ::ItemData BuildCasterEar() const
	{
		auto item = BuildItem(910003, "Fixture Caster Ear", EQ::item::ItemTypeJewelry, 1u << EQ::invslot::slotEar1, 35);
		item.Mana = 250;
		item.AInt = 35;
		item.SpellDmg = 40;
		item.Clairvoyance = 20;
		item.ManaRegen = 8;
		return item;
	}

	EQ::ItemData BuildHealerEar() const
	{
		auto item = BuildItem(910004, "Fixture Healer Ear", EQ::item::ItemTypeJewelry, 1u << EQ::invslot::slotEar2, 35);
		item.Mana = 220;
		item.AWis = 35;
		item.HealAmt = 55;
		item.Clairvoyance = 15;
		item.ManaRegen = 10;
		return item;
	}

	EQ::ItemData BuildSparseDagger() const
	{
		auto item = BuildItem(
			952857,
			"Dagger +4",
			EQ::item::ItemType1HPiercing,
			(1u << EQ::invslot::slotPrimary) | (1u << EQ::invslot::slotSecondary)
		);
		item.Damage = 43;
		item.BackstabDmg = 43;
		item.Delay = 24;
		return item;
	}

	EQ::ItemData BuildTimeweaverHammer() const
	{
		auto item = BuildItem(
			22894,
			"Hammer of the Timeweaver",
			EQ::item::ItemType1HBlunt,
			(1u << EQ::invslot::slotPrimary) | (1u << EQ::invslot::slotSecondary),
			65
		);
		item.Damage = 26;
		item.Delay = 24;
		item.AC = 25;
		item.HP = 220;
		item.Mana = 200;
		item.Endur = 200;
		item.AStr = 25;
		item.ASta = 20;
		item.ADex = 20;
		item.AInt = 15;
		item.AWis = 15;
		item.ACha = 15;
		item.MR = 12;
		item.FR = 12;
		item.CR = 12;
		item.DR = 12;
		item.PR = 12;
		item.Proc.Effect = 3649;
		return item;
	}

	EQ::ItemData BuildWeaponAugment() const
	{
		auto item = BuildItem(910020, "Fixture Weapon Augment", EQ::item::ItemTypeAugmentation, 0);
		item.AugType = EQ::item::AugTypeWeaponGeneral;
		item.Damage = 5;
		item.AC = 15;
		item.HP = 150;
		item.AStr = 12;
		item.ADex = 12;
		item.Accuracy = 10;
		return item;
	}

	void AssertScore(
		const EQ::ItemPower::ScoreResult &score,
		EQ::ItemPower::Role role,
		uint32 item_score,
		uint16 item_level,
		uint32 tank,
		uint32 melee,
		uint32 caster,
		uint32 healer,
		uint32 hybrid
	)
	{
		TEST_ASSERT(score.best_role == role);
		TEST_ASSERT_EQUALS(item_score, score.item_score);
		TEST_ASSERT_EQUALS(item_level, score.item_level);
		TEST_ASSERT_EQUALS(tank, score.tank_score);
		TEST_ASSERT_EQUALS(melee, score.melee_score);
		TEST_ASSERT_EQUALS(caster, score.caster_score);
		TEST_ASSERT_EQUALS(healer, score.healer_score);
		TEST_ASSERT_EQUALS(hybrid, score.hybrid_score);
		TEST_ASSERT(score.source == "computed");
		TEST_ASSERT_EQUALS(EQ::ItemPower::ScoreVersion, score.score_version);
	}

	void NonEquippableItemsAreNotScored()
	{
		auto item = BuildItem(910000, "Fixture Note", EQ::item::ItemTypeNote, 0);

		const auto score = EQ::ItemPower::Calculate(item, true);

		TEST_ASSERT_EQUALS(static_cast<uint16>(0), score.item_level);
		TEST_ASSERT_EQUALS(static_cast<uint32>(0), score.item_score);
		TEST_ASSERT_EQUALS(static_cast<uint32>(0), score.tank_score);
		TEST_ASSERT_EQUALS(static_cast<size_t>(1), score.warnings.size());
		TEST_ASSERT(score.warnings[0] == "item power only scores equippable gear");
		TEST_ASSERT_EQUALS(static_cast<size_t>(1), score.breakdown.size());
		TEST_ASSERT(score.breakdown[0].component == "eligibility");
		TEST_ASSERT_EQUALS(static_cast<int32>(0), score.breakdown[0].score);
	}

	void StableRoleFixturesKeepGoldenScores()
	{
		AssertScore(EQ::ItemPower::Calculate(BuildTankChest(), true), EQ::ItemPower::Role::Tank, 3028, 30, 190, 95, 86, 104, 132);
		AssertScore(EQ::ItemPower::Calculate(BuildMeleeSword(), true), EQ::ItemPower::Role::Melee, 2861, 28, 207, 382, 68, 68, 253);
		AssertScore(EQ::ItemPower::Calculate(BuildCasterEar(), true), EQ::ItemPower::Role::Caster, 3543, 35, 61, 39, 128, 99, 79);
		AssertScore(EQ::ItemPower::Calculate(BuildHealerEar(), true), EQ::ItemPower::Role::Healer, 3549, 35, 69, 44, 92, 146, 86);
	}

	void ProgressionWeightedScoreRanksHigherLevelHammerAboveSimpleDagger()
	{
		const auto dagger = EQ::ItemPower::Calculate(BuildSparseDagger(), true);
		const auto hammer = EQ::ItemPower::Calculate(BuildTimeweaverHammer(), true);

		TEST_ASSERT(hammer.item_level > dagger.item_level);
		TEST_ASSERT(hammer.item_score > dagger.item_score);
		TEST_ASSERT(hammer.item_score >= 6500);
	}

	void AugmentedInstanceScoreIncludesAugments()
	{
		auto base = BuildMeleeSword();
		base.AugSlotType[0] = EQ::item::AugTypeWeaponGeneral;

		const auto augment = BuildWeaponAugment();
		EQ::ItemInstance base_inst(&base, 1);
		EQ::ItemInstance augment_inst(&augment, 1);
		base_inst.PutAugment(0, augment_inst);

		const auto base_score = EQ::ItemPower::Calculate(base, false);
		const auto augmented_score = EQ::ItemPower::Calculate(base_inst, false);

		TEST_ASSERT(augmented_score.source == "instance");
		TEST_ASSERT(augmented_score.item_score > base_score.item_score);
		TEST_ASSERT(augmented_score.melee_score > base_score.melee_score);
	}

	void BreakdownOnlyBuildsWhenRequested()
	{
		const auto with_breakdown = EQ::ItemPower::Calculate(BuildCasterEar(), true);
		TEST_ASSERT_EQUALS(static_cast<size_t>(6), with_breakdown.breakdown.size());
		TEST_ASSERT(with_breakdown.breakdown[0].component == "base_stats");
		TEST_ASSERT_EQUALS(static_cast<int32>(40), with_breakdown.breakdown[0].score);
		TEST_ASSERT(with_breakdown.breakdown[1].component == "defense");
		TEST_ASSERT_EQUALS(static_cast<int32>(0), with_breakdown.breakdown[1].score);
		TEST_ASSERT(with_breakdown.breakdown[2].component == "offense");
		TEST_ASSERT_EQUALS(static_cast<int32>(64), with_breakdown.breakdown[2].score);
		TEST_ASSERT(with_breakdown.breakdown[3].component == "sustain");
		TEST_ASSERT_EQUALS(static_cast<int32>(25), with_breakdown.breakdown[3].score);

		const auto without_breakdown = EQ::ItemPower::Calculate(BuildCasterEar(), false);
		TEST_ASSERT(without_breakdown.breakdown.empty());
		TEST_ASSERT_EQUALS(static_cast<int32>(0), without_breakdown.base_stat_score);
	}

	void RoleNamesKeysAndTransportAreStable()
	{
		TEST_ASSERT(std::string(EQ::ItemPower::RoleName(EQ::ItemPower::Role::Tank)) == "Tank");
		TEST_ASSERT(std::string(EQ::ItemPower::RoleName(EQ::ItemPower::Role::Melee)) == "Melee DPS");
		TEST_ASSERT(std::string(EQ::ItemPower::RoleKey(EQ::ItemPower::Role::Caster)) == "caster");
		TEST_ASSERT(std::string(EQ::ItemPower::RoleKey(EQ::ItemPower::Role::Healer)) == "healer");

		EQ::ItemPower::StoredScore stored;
		stored.item_id = 910010;
		stored.item_level = 44;
		stored.item_score = 222;
		stored.tank_score = 100;
		stored.melee_score = 222;
		stored.caster_score = 80;
		stored.healer_score = 80;
		stored.hybrid_score = 180;
		stored.score_version = EQ::ItemPower::ScoreVersion;
		stored.source = "manual";

		TEST_ASSERT(EQ::ItemPower::BestRoleFromScores(stored) == EQ::ItemPower::Role::Melee);

		auto item = BuildMeleeSword();
		const auto message = EQ::ItemPower::BuildTransportMessage(item, stored);
		TEST_ASSERT(message == "ITEMPOWER|set|item_id=910002|ilvl=44|score=222|role=melee|version=2|source=manual|name=Fixture Melee Sword");
	}
};
