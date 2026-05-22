/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "zone/autoloot_manager.h"
#include "zone/client.h"

void command_autoloot(Client *c, const Seperator *sep)
{
	auto_loot_manager.HandleAutolootCommand(c, sep);
}

void command_lootfilter(Client *c, const Seperator *sep)
{
	auto_loot_manager.HandleLootFilterCommand(c, sep);
}

void command_autosell(Client *c, const Seperator *sep)
{
	auto_loot_manager.HandleAutosellCommand(c, sep);
}

void command_needgreed(Client *c, const Seperator *sep)
{
	auto_loot_manager.HandleNeedGreedCommand(c, sep);
}
