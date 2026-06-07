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
#pragma once

#include "common/types.h"

#include <string>

class Client;

namespace EQ {
	class ItemInstance;
}

enum class ItemRarity : uint8 {
	Common    = 0,
	Uncommon  = 1,
	Rare      = 2,
	Legendary = 3,
	Unique    = 4
};

class ItemRarityManager {
public:
	static bool EnsureSchema();
	static bool SchemaReady();

	static bool TryParseRarity(const std::string &value, ItemRarity &rarity);
	static const char *RarityName(ItemRarity rarity);
	static const char *RarityColorHex(ItemRarity rarity);
	static uint16 RarityChatColor(ItemRarity rarity);

	static bool TryGetRarity(uint32 item_id, ItemRarity &rarity);
	static bool SetRarity(uint32 item_id, ItemRarity rarity);
	static bool ClearRarity(uint32 item_id);

	static std::string BuildItemLink(uint32 item_id);
	static std::string BuildItemLink(uint32 item_id, const char *link_text);
	static std::string BuildItemLink(const EQ::ItemInstance *inst);
	static std::string BuildItemLink(const EQ::ItemInstance *inst, const char *link_text);
	static std::string BuildDecoratedLink(uint32 item_id, ItemRarity rarity);
	static std::string BuildDecoratedLink(const EQ::ItemInstance *inst, ItemRarity rarity);

	static void SendNativeRarity(Client *client, uint32 item_id, ItemRarity rarity);
	static void SendNativeRarityClear(Client *client, uint32 item_id);
	static void SendRarityItemLink(Client *client, uint32 item_id);
	static void SendLootedItemMessage(Client *client, const EQ::ItemInstance *inst, const std::string &item_link);
};
