/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/types.h"

#include <array>

namespace EQ::Multiclass
{
	using ClassSlots = std::array<uint8, 3>;

	bool IsPetClass(uint8 class_id);
	bool IsTankClass(uint8 class_id);
	bool IsHealerClass(uint8 class_id);
	bool IsIntCasterClass(uint8 class_id);
	bool IsWisCasterClass(uint8 class_id);
	bool IsBardClass(uint8 class_id);

	uint32 BuildClassMask(const ClassSlots &class_slots);
	uint32 BuildAAClassMask(const ClassSlots &class_slots);
}
