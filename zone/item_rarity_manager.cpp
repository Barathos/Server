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
#include "item_rarity_manager.h"

#include "common/eq_constants.h"
#include "common/item_data.h"
#include "common/item_instance.h"
#include "common/rulesys.h"
#include "common/say_link.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/zonedb.h"

#include <fmt/format.h>

namespace {
	constexpr const char *kItemRarityTable = "item_rarity";

	int schema_ready = -1;

	void SetSchemaReady(bool ready)
	{
		schema_ready = ready ? 1 : 0;
	}

	ItemRarity ClampRarity(uint8 rarity)
	{
		if (rarity > static_cast<uint8>(ItemRarity::Unique)) {
			return ItemRarity::Common;
		}

		return static_cast<ItemRarity>(rarity);
	}

	std::string ItemName(uint32 item_id)
	{
		const auto *item = database.GetItem(item_id);
		if (!item) {
			return fmt::format("Item {}", item_id);
		}

		return item->Name;
	}

	std::string ItemName(const EQ::ItemInstance *inst)
	{
		if (!inst || !inst->GetItem()) {
			return "Unknown Item";
		}

		return inst->GetItem()->Name;
	}
}

bool ItemRarityManager::EnsureSchema()
{
	const auto results = database.QueryDatabase(
		"CREATE TABLE IF NOT EXISTS `item_rarity` ("
		"`item_id` INT UNSIGNED NOT NULL,"
		"`rarity` TINYINT UNSIGNED NOT NULL DEFAULT 0,"
		"`updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
		"PRIMARY KEY (`item_id`),"
		"CONSTRAINT `item_rarity_rarity_chk` CHECK (`rarity` BETWEEN 0 AND 4)"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"
	);

	SetSchemaReady(results.Success());
	return results.Success();
}

bool ItemRarityManager::SchemaReady()
{
	if (schema_ready != -1) {
		return schema_ready == 1;
	}

	const bool ready = database.DoesTableExist(kItemRarityTable);
	SetSchemaReady(ready);
	return ready;
}

bool ItemRarityManager::TryParseRarity(const std::string &value, ItemRarity &rarity)
{
	const auto lowered = Strings::ToLower(value);

	if (Strings::IsNumber(lowered)) {
		const auto parsed = Strings::ToUnsignedInt(lowered);
		if (parsed <= static_cast<uint32>(ItemRarity::Unique)) {
			rarity = static_cast<ItemRarity>(parsed);
			return true;
		}

		return false;
	}

	if (lowered == "common") {
		rarity = ItemRarity::Common;
		return true;
	}

	if (lowered == "uncommon") {
		rarity = ItemRarity::Uncommon;
		return true;
	}

	if (lowered == "rare") {
		rarity = ItemRarity::Rare;
		return true;
	}

	if (lowered == "legendary") {
		rarity = ItemRarity::Legendary;
		return true;
	}

	if (lowered == "unique") {
		rarity = ItemRarity::Unique;
		return true;
	}

	return false;
}

const char *ItemRarityManager::RarityName(ItemRarity rarity)
{
	switch (rarity) {
		case ItemRarity::Common:
			return "Common";
		case ItemRarity::Uncommon:
			return "Uncommon";
		case ItemRarity::Rare:
			return "Rare";
		case ItemRarity::Legendary:
			return "Legendary";
		case ItemRarity::Unique:
			return "Unique";
	}

	return "Common";
}

const char *ItemRarityManager::RarityColorHex(ItemRarity rarity)
{
	switch (rarity) {
		case ItemRarity::Common:
			return "#F0F0F0";
		case ItemRarity::Uncommon:
			return "#66FF66";
		case ItemRarity::Rare:
			return "#00FFFF";
		case ItemRarity::Legendary:
			return "#FFD15C";
		case ItemRarity::Unique:
			return "#C080FF";
	}

	return "#F0F0F0";
}

uint16 ItemRarityManager::RarityChatColor(ItemRarity rarity)
{
	switch (rarity) {
		case ItemRarity::Common:
			return Chat::White;
		case ItemRarity::Uncommon:
			return Chat::Green;
		case ItemRarity::Rare:
			return Chat::Cyan;
		case ItemRarity::Legendary:
			return Chat::Yellow;
		case ItemRarity::Unique:
			return Chat::Magenta;
	}

	return Chat::White;
}

bool ItemRarityManager::TryGetRarity(uint32 item_id, ItemRarity &rarity)
{
	if (!item_id || !SchemaReady()) {
		return false;
	}

	auto results = database.QueryDatabase(
		fmt::format(
			"SELECT `rarity` FROM `{}` WHERE `item_id` = {} LIMIT 1",
			kItemRarityTable,
			item_id
		)
	);

	if (!results.Success() || !results.RowCount()) {
		return false;
	}

	auto row = results.begin();
	if (!row[0]) {
		return false;
	}

	rarity = ClampRarity(static_cast<uint8>(Strings::ToUnsignedInt(row[0])));
	return true;
}

bool ItemRarityManager::SetRarity(uint32 item_id, ItemRarity rarity)
{
	if (!item_id || !database.GetItem(item_id)) {
		return false;
	}

	if (!SchemaReady() && !EnsureSchema()) {
		return false;
	}

	const auto results = database.QueryDatabase(
		fmt::format(
			"INSERT INTO `{}` (`item_id`, `rarity`, `updated_at`) VALUES ({}, {}, NOW()) "
			"ON DUPLICATE KEY UPDATE `rarity` = VALUES(`rarity`), `updated_at` = NOW()",
			kItemRarityTable,
			item_id,
			static_cast<uint8>(rarity)
		)
	);

	return results.Success();
}

bool ItemRarityManager::ClearRarity(uint32 item_id)
{
	if (!item_id || !SchemaReady()) {
		return false;
	}

	const auto results = database.QueryDatabase(
		fmt::format(
			"DELETE FROM `{}` WHERE `item_id` = {}",
			kItemRarityTable,
			item_id
		)
	);

	return results.Success();
}

std::string ItemRarityManager::BuildItemLink(uint32 item_id)
{
	return BuildItemLink(item_id, nullptr);
}

std::string ItemRarityManager::BuildItemLink(uint32 item_id, const char *link_text)
{
	const auto *item = database.GetItem(item_id);
	if (!item) {
		return fmt::format("Item {}", item_id);
	}

	EQ::SayLinkEngine linker;
	linker.SetLinkType(EQ::saylink::SayLinkItemData);
	linker.SetItemData(item);
	if (link_text && link_text[0]) {
		linker.SetProxyText(link_text);
	}

	return linker.GenerateLink();
}

std::string ItemRarityManager::BuildItemLink(const EQ::ItemInstance *inst)
{
	return BuildItemLink(inst, nullptr);
}

std::string ItemRarityManager::BuildItemLink(const EQ::ItemInstance *inst, const char *link_text)
{
	if (!inst || !inst->GetItem()) {
		return "Unknown Item";
	}

	EQ::SayLinkEngine linker;
	linker.SetLinkType(EQ::saylink::SayLinkItemInst);
	linker.SetItemInst(inst);
	if (link_text && link_text[0]) {
		linker.SetProxyText(link_text);
	}

	return linker.GenerateLink();
}

std::string ItemRarityManager::BuildDecoratedLink(uint32 item_id, ItemRarity rarity)
{
	return fmt::format(
		"[{}] {} {}",
		RarityName(rarity),
		ItemName(item_id),
		BuildItemLink(item_id, "(inspect)")
	);
}

std::string ItemRarityManager::BuildDecoratedLink(const EQ::ItemInstance *inst, ItemRarity rarity)
{
	return fmt::format(
		"[{}] {} {}",
		RarityName(rarity),
		ItemName(inst),
		BuildItemLink(inst, "(inspect)")
	);
}

void ItemRarityManager::SendNativeRarity(Client *client, uint32 item_id, ItemRarity rarity)
{
	if (!RuleB(CustomFeatures, ItemRarityEnabled)) {
		return;
	}

	if (!client || !item_id) {
		return;
	}

	const auto *item = database.GetItem(item_id);
	client->Message(
		Chat::Black,
		"%s",
		fmt::format(
			"ITEMRARITY|set|item_id={}|rarity={}|name={}",
			item_id,
			static_cast<uint8>(rarity),
			item ? item->Name : ""
		).c_str()
	);
}

void ItemRarityManager::SendNativeRarityClear(Client *client, uint32 item_id)
{
	if (!RuleB(CustomFeatures, ItemRarityEnabled)) {
		return;
	}

	if (!client || !item_id) {
		return;
	}

	const auto *item = database.GetItem(item_id);
	client->Message(
		Chat::Black,
		"%s",
		fmt::format(
			"ITEMRARITY|clear|item_id={}|name={}",
			item_id,
			item ? item->Name : ""
		).c_str()
	);
}

void ItemRarityManager::SendRarityItemLink(Client *client, uint32 item_id)
{
	if (!RuleB(CustomFeatures, ItemRarityEnabled)) {
		return;
	}

	if (!client) {
		return;
	}

	ItemRarity rarity = ItemRarity::Common;
	const bool has_rarity = TryGetRarity(item_id, rarity);
	if (!has_rarity) {
		client->Message(
			Chat::White,
			"%s",
			fmt::format(
				"{} has no explicit rarity tag. Default display is Common.",
				BuildItemLink(item_id)
			).c_str()
		);
		return;
	}

	SendNativeRarity(client, item_id, rarity);
	client->Message(
		RarityChatColor(rarity),
		"%s",
		BuildDecoratedLink(item_id, rarity).c_str()
	);
}

void ItemRarityManager::SendLootedItemMessage(Client *client, const EQ::ItemInstance *inst, const std::string &)
{
	if (!RuleB(CustomFeatures, ItemRarityEnabled)) {
		return;
	}

	if (!client || !inst || !inst->GetItem()) {
		return;
	}

	ItemRarity rarity = ItemRarity::Common;
	if (!TryGetRarity(inst->GetItem()->ID, rarity)) {
		return;
	}

	SendNativeRarity(client, inst->GetItem()->ID, rarity);
	client->Message(
		RarityChatColor(rarity),
		"%s",
		fmt::format(
			"Looted {}",
			BuildDecoratedLink(inst, rarity)
		).c_str()
	);
}
