/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.
*/
#include "zone/client.h"
#include "zone/object.h"

#include "common/seperator.h"
#include "common/strings.h"

namespace {

void SendTradeskillCommandUsage(Client *client)
{
	client->Message(Chat::White, "Tradeskill commands:");
	client->Message(Chat::White, "#ts makeall - Make all available combines for the selected/last tradeskill recipe.");
	client->Message(Chat::White, "#ts makeall <limit> - Make up to <limit> combines for the selected/last recipe.");
	client->Message(Chat::White, "#ts makeall <recipe_id> <object_type> <some_id> [limit] - Make all for an explicit recipe tuple.");
}

} // namespace

void command_tradeskill(Client *c, const Seperator *sep)
{
	if (!c) {
		return;
	}

	const std::string action = sep && sep->arg[1] ? Strings::ToLower(sep->arg[1]) : "";
	if (action.empty() || action == "help") {
		SendTradeskillCommandUsage(c);
		return;
	}

	if (action != "makeall" && action != "all") {
		SendTradeskillCommandUsage(c);
		return;
	}

	RecipeAutoCombine_Struct recipe = {};
	uint32 limit = 0;

	const std::string first_value = sep->arg[2] ? Strings::ToLower(sep->arg[2]) : "";
	if (
		!first_value.empty() &&
		Strings::IsNumber(first_value) &&
		sep->arg[3] && sep->arg[3][0] &&
		sep->arg[4] && sep->arg[4][0]
	) {
		recipe.recipe_id = Strings::ToUnsignedInt(sep->arg[2]);
		recipe.object_type = Strings::ToUnsignedInt(sep->arg[3]);
		recipe.some_id = Strings::ToUnsignedInt(sep->arg[4]);
		limit = sep->arg[5] && Strings::IsNumber(sep->arg[5]) ? Strings::ToUnsignedInt(sep->arg[5]) : 0;
		c->SetLastRecipeAutoCombine(&recipe);
	} else {
		if (!first_value.empty() && Strings::IsNumber(first_value)) {
			limit = Strings::ToUnsignedInt(first_value);
		} else if (first_value == "last" && sep->arg[3] && Strings::IsNumber(sep->arg[3])) {
			limit = Strings::ToUnsignedInt(sep->arg[3]);
		} else if (!first_value.empty() && first_value != "last") {
			SendTradeskillCommandUsage(c);
			return;
		}

		if (!c->GetLastRecipeAutoCombine(recipe)) {
			c->Message(Chat::White, "Select a learned recipe in the tradeskill window, then use #ts makeall.");
			return;
		}
	}

	Object::HandleAutoCombineAll(c, &recipe, limit);
}
