/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "zone/client.h"

void command_aaexp(Client *c, const Seperator *sep)
{
	if (!c) {
		return;
	}

	if (!sep || sep->argnum < 1 || !sep->arg[1][0] || Strings::EqualFold(sep->arg[1], "status")) {
		c->Message(Chat::White, "AA experience allocation is currently %u%%. Usage: #aaexp <0-100>", c->GetAAEXPPercentage());
		return;
	}

	if (Strings::EqualFold(sep->arg[1], "off")) {
		c->SetAAEXPPercentage(0);
		c->Message(Chat::White, "AA experience allocation set to 0%%.");
		return;
	}

	if (!sep->IsNumber(1)) {
		c->Message(Chat::White, "Usage: #aaexp <0-100>");
		return;
	}

	const int percentage = Strings::ToInt(sep->arg[1]);
	if (percentage < 0 || percentage > 100) {
		c->Message(Chat::White, "AA experience allocation must be between 0 and 100.");
		return;
	}

	if (c->GetLevel() < 51 && !RuleB(AA, AllowAAExpBelowLevel51)) {
		c->SetAAEXPPercentage(0);
		c->Message(Chat::Yellow, "You are below the level allowed to gain AA Experience.");
		return;
	}

	c->SetAAEXPPercentage(static_cast<uint8>(percentage));
	c->Message(Chat::White, "AA experience allocation set to %u%%.", percentage);
}
