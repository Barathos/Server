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

#include "common/item_instance.h"
#include "common/strings.h"
#include "cppunit/cpptest.h"

class DynamicItemTest : public Test::Suite {
	typedef void(DynamicItemTest::*TestFunction)(void);
public:
	DynamicItemTest() {
		TEST_ADD(DynamicItemTest::DynamicModifiersApplyToResolvedItem);
		TEST_ADD(DynamicItemTest::DynamicDataPersistsThroughCustomData);
		TEST_ADD(DynamicItemTest::DynamicDataCopiesAndClears);
		TEST_ADD(DynamicItemTest::DynamicDataChangesSerialNumber);
		TEST_ADD(DynamicItemTest::DynamicDataRebuildsAfterBaseRefresh);
	}

private:
	EQ::ItemData BuildSword() const
	{
		EQ::ItemData item{};
		item.ItemClass = EQ::item::ItemClassCommon;
		item.ItemType = EQ::item::ItemType1HSlash;
		item.ID = 900001;
		item.Damage = 10;
		item.Delay = 20;
		item.AStr = 2;
		item.HP = 10;
		item.Mana = 0;
		item.Haste = 0;
		item.MaxCharges = 1;
		item.NoDrop = 255;
		item.NoRent = 255;
		strn0cpy(item.Name, "Training Sword", sizeof(item.Name));
		strn0cpy(item.Lore, "A plain blade", sizeof(item.Lore));
		strn0cpy(item.IDFile, "IT10", sizeof(item.IDFile));
		return item;
	}

	void DynamicModifiersApplyToResolvedItem()
	{
		auto base = BuildSword();
		EQ::ItemInstance inst(&base, 1);

		TEST_ASSERT(!inst.HasDynamicItemData());

		inst.SetDynamicItemModifier("hp", 50);
		inst.SetDynamicItemModifier("mana", 35);
		inst.SetDynamicItemModifier("agi", 7);
		inst.SetDynamicItemData("haste", 18);
		inst.SetDynamicItemData("proc_effect", 1234);
		inst.SetDynamicItemData("proc_type", EQ::item::ItemEffectCombatProc);
		inst.SetDynamicItemData("proc_level", 1);
		inst.SetDynamicItemData("proc_level2", 255);
		inst.SetDynamicItemData("proc_rate", 150);
		inst.SetDynamicItemData("proc_name", "Spark");
		inst.SetDynamicItemData("name", "Training Sword of Sparks");

		const auto *resolved = inst.GetItem();
		TEST_ASSERT(inst.HasDynamicItemData());
		TEST_ASSERT(resolved != nullptr);
		TEST_ASSERT_EQUALS(resolved->HP, 60);
		TEST_ASSERT_EQUALS(resolved->Mana, 35);
		TEST_ASSERT_EQUALS(static_cast<int>(resolved->AAgi), 7);
		TEST_ASSERT_EQUALS(resolved->Haste, 18);
		TEST_ASSERT_EQUALS(resolved->Proc.Effect, 1234);
		TEST_ASSERT_EQUALS(static_cast<int>(resolved->Proc.Type), static_cast<int>(EQ::item::ItemEffectCombatProc));
		TEST_ASSERT_EQUALS(static_cast<int>(resolved->Proc.Level), 1);
		TEST_ASSERT_EQUALS(static_cast<int>(resolved->Proc.Level2), 255);
		TEST_ASSERT_EQUALS(resolved->ProcRate, 150);
		TEST_ASSERT(std::string(resolved->ProcName) == "Spark");
		TEST_ASSERT(std::string(resolved->Name) == "Training Sword of Sparks");

		TEST_ASSERT_EQUALS(inst.GetUnscaledItem()->HP, 10);
		TEST_ASSERT_EQUALS(inst.GetUnscaledItem()->Mana, 0);
		TEST_ASSERT(inst.GetClientItem() == resolved);
	}

	void DynamicDataPersistsThroughCustomData()
	{
		auto base = BuildSword();
		EQ::ItemInstance inst(&base, 1);
		inst.SetDynamicItemModifier("hp", 75);
		inst.SetDynamicItemData("haste", 22);
		inst.SetDynamicItemData("worn_effect", 4321);

		EQ::ItemInstance loaded(&base, 1);
		loaded.SetCustomDataString(inst.GetCustomDataString());

		TEST_ASSERT(loaded.HasDynamicItemData());
		TEST_ASSERT_EQUALS(loaded.GetItem()->HP, 85);
		TEST_ASSERT_EQUALS(loaded.GetItem()->Haste, 22);
		TEST_ASSERT_EQUALS(loaded.GetItem()->Worn.Effect, 4321);
		TEST_ASSERT_EQUALS(loaded.GetUnscaledItem()->HP, 10);
	}

	void DynamicDataCopiesAndClears()
	{
		auto base = BuildSword();
		EQ::ItemInstance inst(&base, 1);
		inst.SetDynamicItemModifier("hp", 20);
		inst.SetDynamicItemData("damage", 14);

		EQ::ItemInstance copy(inst);
		TEST_ASSERT(copy.HasDynamicItemData());
		TEST_ASSERT_EQUALS(copy.GetItem()->HP, 30);
		TEST_ASSERT_EQUALS(copy.GetItem()->Damage, static_cast<uint32>(14));

		copy.ClearDynamicItemData();
		TEST_ASSERT(!copy.HasDynamicItemData());
		TEST_ASSERT_EQUALS(copy.GetItem()->HP, 10);
		TEST_ASSERT_EQUALS(copy.GetItem()->Damage, static_cast<uint32>(10));
	}

	void DynamicDataChangesSerialNumber()
	{
		auto base = BuildSword();
		EQ::ItemInstance inst(&base, 1);

		const auto initial_serial = inst.GetSerialNumber();
		inst.SetCustomData("note", "no client item change");
		TEST_ASSERT_EQUALS(initial_serial, inst.GetSerialNumber());

		inst.SetDynamicItemModifier("hp", 20);
		const auto modified_serial = inst.GetSerialNumber();
		TEST_ASSERT(initial_serial != modified_serial);

		inst.DeleteDynamicItemModifier("hp");
		TEST_ASSERT(modified_serial != inst.GetSerialNumber());
	}

	void DynamicDataRebuildsAfterBaseRefresh()
	{
		auto base = BuildSword();
		EQ::ItemInstance inst(&base, 1);
		inst.SetDynamicItemModifier("hp", 50);
		inst.SetDynamicItemData("damage", 14);
		const auto serial_before_refresh = inst.GetSerialNumber();

		base.HP = 40;
		base.Mana = 25;
		base.Damage = 12;
		strn0cpy(base.Name, "Training Sword Mk II", sizeof(base.Name));

		TEST_ASSERT(inst.RefreshItemData(&base));
		TEST_ASSERT_EQUALS(inst.GetUnscaledItem()->HP, 40);
		TEST_ASSERT_EQUALS(inst.GetUnscaledItem()->Mana, 25);
		TEST_ASSERT(std::string(inst.GetUnscaledItem()->Name) == "Training Sword Mk II");
		TEST_ASSERT_EQUALS(inst.GetItem()->HP, 90);
		TEST_ASSERT_EQUALS(inst.GetItem()->Mana, 25);
		TEST_ASSERT_EQUALS(inst.GetItem()->Damage, static_cast<uint32>(14));
		TEST_ASSERT(std::string(inst.GetItem()->Name) == "Training Sword Mk II");
		TEST_ASSERT(serial_before_refresh != inst.GetSerialNumber());

		const auto serial_after_refresh = inst.GetSerialNumber();
		TEST_ASSERT(!inst.RefreshItemData(&base));
		TEST_ASSERT_EQUALS(serial_after_refresh, inst.GetSerialNumber());
	}
};
