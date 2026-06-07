/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#pragma once

#include "common/types.h"

#include <string>

class Client;

namespace LiveSpellManager {
	void EnsureServerLoaded();
	void SendClientSync(Client *c);
	bool CreateSpellScroll(
		Client *c,
		const std::string &element,
		const std::string &target,
		int range,
		int damage,
		int recast_time,
		const std::string &custom_name = ""
	);
	bool IsLiveSpell(uint16 spell_id);
}
