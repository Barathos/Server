/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "multiclass_logic.h"

#include "common/classes.h"

bool EQ::Multiclass::IsPetClass(uint8 class_id)
{
	return class_id == Class::Magician || class_id == Class::Necromancer || class_id == Class::Beastlord;
}

bool EQ::Multiclass::IsTankClass(uint8 class_id)
{
	return class_id == Class::Warrior || class_id == Class::Paladin || class_id == Class::ShadowKnight;
}

bool EQ::Multiclass::IsHealerClass(uint8 class_id)
{
	return class_id == Class::Cleric || class_id == Class::Druid || class_id == Class::Shaman;
}

bool EQ::Multiclass::IsIntCasterClass(uint8 class_id)
{
	switch (class_id) {
		case Class::ShadowKnight:
		case Class::Bard:
		case Class::Necromancer:
		case Class::Wizard:
		case Class::Magician:
		case Class::Enchanter:
			return true;
		default:
			return false;
	}
}

bool EQ::Multiclass::IsWisCasterClass(uint8 class_id)
{
	switch (class_id) {
		case Class::Cleric:
		case Class::Paladin:
		case Class::Ranger:
		case Class::Druid:
		case Class::Shaman:
		case Class::Beastlord:
			return true;
		default:
			return false;
	}
}

bool EQ::Multiclass::IsBardClass(uint8 class_id)
{
	return class_id == Class::Bard;
}

uint32 EQ::Multiclass::BuildClassMask(const ClassSlots &class_slots)
{
	uint32 mask = 0;
	for (const auto class_id : class_slots) {
		if (IsPlayerClass(class_id)) {
			mask |= GetPlayerClassBit(class_id);
		}
	}

	return mask;
}

uint32 EQ::Multiclass::BuildAAClassMask(const ClassSlots &class_slots)
{
	uint32 mask = 0;
	for (const auto class_id : class_slots) {
		if (IsPlayerClass(class_id)) {
			mask |= (1 << class_id);
		}
	}

	return mask;
}
