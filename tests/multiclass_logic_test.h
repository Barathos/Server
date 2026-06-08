/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/classes.h"
#include "common/multiclass_logic.h"
#include "cppunit/cpptest.h"

class MulticlassLogicTest : public Test::Suite {
public:
	MulticlassLogicTest()
	{
		TEST_ADD(MulticlassLogicTest::ClassMasksIgnoreEmptyAndInvalidSlots);
		TEST_ADD(MulticlassLogicTest::AAClassMasksFollowClassSlotIntent);
		TEST_ADD(MulticlassLogicTest::ClassCategoriesMatchManagerBehavior);
		TEST_ADD(MulticlassLogicTest::MixedTrioFixturesCoverExpectedCapabilities);
	}

private:
	using ClassSlots = EQ::Multiclass::ClassSlots;

	void ClassMasksIgnoreEmptyAndInvalidSlots()
	{
		const ClassSlots slots = {Class::Warrior, Class::None, Class::FellowshipMaster};
		TEST_ASSERT_EQUALS(
			static_cast<uint32>(GetPlayerClassBit(Class::Warrior)),
			EQ::Multiclass::BuildClassMask(slots)
		);

		const ClassSlots mixed = {Class::Warrior, Class::Cleric, Class::Wizard};
		const uint32 expected =
			GetPlayerClassBit(Class::Warrior) |
			GetPlayerClassBit(Class::Cleric) |
			GetPlayerClassBit(Class::Wizard);
		TEST_ASSERT_EQUALS(expected, EQ::Multiclass::BuildClassMask(mixed));
	}

	void AAClassMasksFollowClassSlotIntent()
	{
		const ClassSlots slots = {Class::ShadowKnight, Class::Bard, Class::Necromancer};
		const uint32 expected =
			(1 << Class::ShadowKnight) |
			(1 << Class::Bard) |
			(1 << Class::Necromancer);
		TEST_ASSERT_EQUALS(expected, EQ::Multiclass::BuildAAClassMask(slots));

		const ClassSlots invalid = {Class::None, Class::FellowshipMaster, Class::BerserkerGM};
		TEST_ASSERT_EQUALS(static_cast<uint32>(0), EQ::Multiclass::BuildAAClassMask(invalid));
	}

	void ClassCategoriesMatchManagerBehavior()
	{
		TEST_ASSERT(EQ::Multiclass::IsTankClass(Class::Warrior));
		TEST_ASSERT(EQ::Multiclass::IsTankClass(Class::Paladin));
		TEST_ASSERT(EQ::Multiclass::IsTankClass(Class::ShadowKnight));
		TEST_ASSERT(!EQ::Multiclass::IsTankClass(Class::Monk));

		TEST_ASSERT(EQ::Multiclass::IsHealerClass(Class::Cleric));
		TEST_ASSERT(EQ::Multiclass::IsHealerClass(Class::Druid));
		TEST_ASSERT(EQ::Multiclass::IsHealerClass(Class::Shaman));
		TEST_ASSERT(!EQ::Multiclass::IsHealerClass(Class::Paladin));

		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(Class::ShadowKnight));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(Class::Bard));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(Class::Necromancer));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(Class::Wizard));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(Class::Magician));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(Class::Enchanter));
		TEST_ASSERT(!EQ::Multiclass::IsIntCasterClass(Class::Cleric));

		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(Class::Cleric));
		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(Class::Paladin));
		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(Class::Ranger));
		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(Class::Druid));
		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(Class::Shaman));
		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(Class::Beastlord));
		TEST_ASSERT(!EQ::Multiclass::IsWisCasterClass(Class::Wizard));

		TEST_ASSERT(EQ::Multiclass::IsBardClass(Class::Bard));
		TEST_ASSERT(!EQ::Multiclass::IsBardClass(Class::Rogue));

		TEST_ASSERT(EQ::Multiclass::IsPetClass(Class::Magician));
		TEST_ASSERT(EQ::Multiclass::IsPetClass(Class::Necromancer));
		TEST_ASSERT(EQ::Multiclass::IsPetClass(Class::Beastlord));
		TEST_ASSERT(!EQ::Multiclass::IsPetClass(Class::Enchanter));
	}

	void MixedTrioFixturesCoverExpectedCapabilities()
	{
		const ClassSlots classic = {Class::Warrior, Class::Cleric, Class::Wizard};
		TEST_ASSERT(EQ::Multiclass::BuildClassMask(classic) & GetPlayerClassBit(Class::Warrior));
		TEST_ASSERT(EQ::Multiclass::BuildClassMask(classic) & GetPlayerClassBit(Class::Cleric));
		TEST_ASSERT(EQ::Multiclass::BuildClassMask(classic) & GetPlayerClassBit(Class::Wizard));
		TEST_ASSERT(EQ::Multiclass::IsTankClass(classic[0]));
		TEST_ASSERT(EQ::Multiclass::IsHealerClass(classic[1]));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(classic[2]));

		const ClassSlots dirge = {Class::ShadowKnight, Class::Bard, Class::Necromancer};
		TEST_ASSERT(EQ::Multiclass::IsTankClass(dirge[0]));
		TEST_ASSERT(EQ::Multiclass::IsBardClass(dirge[1]));
		TEST_ASSERT(EQ::Multiclass::IsPetClass(dirge[2]));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(dirge[0]));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(dirge[1]));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(dirge[2]));

		const ClassSlots pet_control = {Class::Magician, Class::Beastlord, Class::Enchanter};
		TEST_ASSERT(EQ::Multiclass::IsPetClass(pet_control[0]));
		TEST_ASSERT(EQ::Multiclass::IsPetClass(pet_control[1]));
		TEST_ASSERT(!EQ::Multiclass::IsPetClass(pet_control[2]));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(pet_control[0]));
		TEST_ASSERT(EQ::Multiclass::IsWisCasterClass(pet_control[1]));
		TEST_ASSERT(EQ::Multiclass::IsIntCasterClass(pet_control[2]));
	}
};
