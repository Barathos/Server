/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "common/item_instance.h"
#include "common/rulesys.h"
#include "common/seperator.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/corpse.h"
#include "zone/item_rarity_manager.h"
#include "zone/npc.h"
#include "zone/zonedb.h"

#include <fmt/format.h>

namespace {
	void SendUsage(Client *c)
	{
		c->Message(Chat::White, "Usage: #itemrarity init - Create the item_rarity table if it is missing.");
		c->Message(Chat::White, "Usage: #itemrarity legend - Show rarity color samples.");
		c->Message(Chat::White, "Usage: #itemrarity set [Item ID] [common|uncommon|rare|legendary|unique] - Tag an item.");
		c->Message(Chat::White, "Usage: #itemrarity clear [Item ID] - Remove an item's rarity tag.");
		c->Message(Chat::White, "Usage: #itemrarity show [Item ID] - Show an item's rarity tag.");
		c->Message(Chat::White, "Usage: #itemrarity link [Item ID] - Send a clickable rarity-colored item link.");
		c->Message(Chat::White, "Usage: #itemrarity view [Item ID] - Open the normal item inspect window and send its rarity link.");
		c->Message(Chat::White, "Usage: #itemrarity loot [Item ID] [Rarity] [Charges] - Add tagged test loot to your NPC or corpse target.");
	}

	bool GetItemID(Client *c, const Seperator *sep, int argument, uint32 &item_id)
	{
		if (!sep->IsNumber(argument)) {
			c->Message(Chat::White, "Item ID must be numeric.");
			return false;
		}

		item_id = Strings::ToUnsignedInt(sep->arg[argument]);
		if (!database.GetItem(item_id)) {
			c->Message(
				Chat::White,
				"%s",
				fmt::format(
					"Item ID {} does not exist.",
					item_id
				).c_str()
			);
			return false;
		}

		return true;
	}

	bool GetRarity(Client *c, const Seperator *sep, int argument, ItemRarity &rarity)
	{
		if (!ItemRarityManager::TryParseRarity(sep->arg[argument], rarity)) {
			c->Message(Chat::White, "Rarity must be common, uncommon, rare, legendary, unique, or a value from 0 to 4.");
			return false;
		}

		return true;
	}

	void SendLegend(Client *c)
	{
		const ItemRarity rarities[] = {
			ItemRarity::Common,
			ItemRarity::Uncommon,
			ItemRarity::Rare,
			ItemRarity::Legendary,
			ItemRarity::Unique
		};

		for (const auto rarity : rarities) {
			c->Message(
				ItemRarityManager::RarityChatColor(rarity),
				"%s",
				fmt::format(
					"{} ({})",
					ItemRarityManager::RarityName(rarity),
					ItemRarityManager::RarityColorHex(rarity)
				).c_str()
			);
		}
	}

	void SendItemView(Client *c, uint32 item_id)
	{
		auto *inst = database.CreateItem(item_id);
		if (!inst) {
			c->Message(Chat::White, "Unable to create item instance.");
			return;
		}

		c->SendItemPacket(0, inst, ItemPacketViewLink);
		delete inst;
	}
}

void command_itemrarity(Client *c, const Seperator *sep)
{
	if (!RuleB(CustomFeatures, ItemRarityEnabled)) {
		c->Message(Chat::White, "Item Rarity is disabled on this server.");
		return;
	}

	const auto arguments = sep->argnum;
	if (!arguments || !strcasecmp(sep->arg[1], "help")) {
		SendUsage(c);
		return;
	}

	const bool is_init   = !strcasecmp(sep->arg[1], "init");
	const bool is_legend = !strcasecmp(sep->arg[1], "legend");
	const bool is_set    = !strcasecmp(sep->arg[1], "set");
	const bool is_clear  = !strcasecmp(sep->arg[1], "clear");
	const bool is_show   = !strcasecmp(sep->arg[1], "show");
	const bool is_link   = !strcasecmp(sep->arg[1], "link");
	const bool is_view   = !strcasecmp(sep->arg[1], "view");
	const bool is_loot   = !strcasecmp(sep->arg[1], "loot");

	if (is_init) {
		if (ItemRarityManager::EnsureSchema()) {
			c->Message(Chat::Green, "Item rarity schema is ready.");
		}
		else {
			c->Message(Chat::Red, "Failed to create item rarity schema. Check zone logs for database errors.");
		}

		return;
	}

	if (is_legend) {
		SendLegend(c);
		return;
	}

	if (is_set) {
		if (arguments < 3) {
			c->Message(Chat::White, "Usage: #itemrarity set [Item ID] [common|uncommon|rare|legendary|unique]");
			return;
		}

		uint32 item_id = 0;
		if (!GetItemID(c, sep, 2, item_id)) {
			return;
		}

		ItemRarity rarity = ItemRarity::Common;
		if (!GetRarity(c, sep, 3, rarity)) {
			return;
		}

		if (!ItemRarityManager::SetRarity(item_id, rarity)) {
			c->Message(Chat::Red, "Failed to save item rarity.");
			return;
		}

		ItemRarityManager::SendNativeRarity(c, item_id, rarity);
		c->Message(
			ItemRarityManager::RarityChatColor(rarity),
			"%s",
			fmt::format(
				"Tagged {}.",
				ItemRarityManager::BuildDecoratedLink(item_id, rarity)
			).c_str()
		);
		return;
	}

	if (is_clear) {
		if (arguments < 2) {
			c->Message(Chat::White, "Usage: #itemrarity clear [Item ID]");
			return;
		}

		uint32 item_id = 0;
		if (!GetItemID(c, sep, 2, item_id)) {
			return;
		}

		if (!ItemRarityManager::ClearRarity(item_id)) {
			c->Message(Chat::Red, "Failed to clear item rarity.");
			return;
		}

		ItemRarityManager::SendNativeRarityClear(c, item_id);
		c->Message(
			Chat::White,
			"%s",
			fmt::format(
				"Cleared rarity tag for {}.",
				ItemRarityManager::BuildItemLink(item_id)
			).c_str()
		);
		return;
	}

	if (is_show || is_link || is_view) {
		if (arguments < 2) {
			c->Message(Chat::White, "Usage: #itemrarity show|link|view [Item ID]");
			return;
		}

		uint32 item_id = 0;
		if (!GetItemID(c, sep, 2, item_id)) {
			return;
		}

		if (is_view) {
			ItemRarity rarity = ItemRarity::Common;
			if (ItemRarityManager::TryGetRarity(item_id, rarity)) {
				ItemRarityManager::SendNativeRarity(c, item_id, rarity);
			}

			SendItemView(c, item_id);
		}

		ItemRarityManager::SendRarityItemLink(c, item_id);
		return;
	}

	if (is_loot) {
		if (arguments < 3) {
			c->Message(Chat::White, "Usage: #itemrarity loot [Item ID] [Rarity] [Charges]");
			return;
		}

		if (!c->GetTarget() || (!c->GetTarget()->IsNPC() && !c->GetTarget()->IsCorpse())) {
			c->Message(Chat::White, "Target an NPC or corpse before using #itemrarity loot.");
			return;
		}

		uint32 item_id = 0;
		if (!GetItemID(c, sep, 2, item_id)) {
			return;
		}

		ItemRarity rarity = ItemRarity::Common;
		if (!GetRarity(c, sep, 3, rarity)) {
			return;
		}

		const auto charges = sep->IsNumber(4) ? static_cast<uint16>(Strings::ToUnsignedInt(sep->arg[4])) : static_cast<uint16>(1);
		if (!ItemRarityManager::SetRarity(item_id, rarity)) {
			c->Message(Chat::Red, "Failed to save item rarity.");
			return;
		}

		auto target = c->GetTarget();
		if (target->IsNPC()) {
			target->CastToNPC()->AddItem(item_id, charges, false);
		}
		else if (target->IsCorpse()) {
			target->CastToCorpse()->AddItem(item_id, charges);
		}

		ItemRarityManager::SendNativeRarity(c, item_id, rarity);
		c->Message(
			ItemRarityManager::RarityChatColor(rarity),
			"%s",
			fmt::format(
				"Added {} to {}.",
				ItemRarityManager::BuildDecoratedLink(item_id, rarity),
				c->GetTargetDescription(target)
			).c_str()
		);
		return;
	}

	SendUsage(c);
}
