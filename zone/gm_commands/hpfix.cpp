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
#include "common/rulesys.h"
#include "zone/client.h"

namespace {
	constexpr uint32 HPFIX_CHARM_ITEM_ID = 990001;
	constexpr uint32 HPFIX_CHEST_ITEM_ID = 990002;
	constexpr uint32 HPFIX_WAIST_ITEM_ID = 990003;

	bool HpFixCanUseGmTools(Client* c)
	{
		return c && c->Admin() >= AccountStatus::GMMgmt;
	}

	void HpFixSummonItem(Client* c, uint32 item_id)
	{
		const auto* item = database.GetItem(item_id);
		if (!item) {
			c->Message(
				Chat::White,
				fmt::format(
					"HPFIX test item {} is missing. Apply features/hpfix/sql/001_hpfix_test_items.sql first.",
					item_id
				).c_str()
			);
			return;
		}

		c->SummonItem(item_id);
		c->Message(Chat::White, fmt::format("Summoned {}.", item->Name).c_str());
	}

	void HpFixShowUsage(Client* c)
	{
		c->Message(Chat::White, "HPFIX: native ready, refresh, status, window, off, items");
		c->Message(Chat::White, "The DLL sends '#hpfix native ready' automatically; players should not need to type this.");
	}
}

void command_hpfix(Client* c, const Seperator* sep)
{
	if (!RuleB(CustomFeatures, HpFixEnabled)) {
		c->SetNativeHpFixReady(false);
		c->Message(Chat::White, "HPFIX is disabled on this server.");
		return;
	}

	const uint16 arguments = sep->argnum;

	if (
		arguments >= 2 &&
		!strcasecmp(sep->arg[1], "native") &&
		!strcasecmp(sep->arg[2], "ready")
	) {
		c->SetNativeHpFixReady(true);
		c->SendNativeHpFixUpdate(true);
		return;
	}

	if (!arguments || !strcasecmp(sep->arg[1], "help")) {
		HpFixShowUsage(c);
		return;
	}

	if (!strcasecmp(sep->arg[1], "refresh")) {
		c->SetNativeHpFixReady(true);
		c->SendNativeHpFixUpdate(true);
		c->Message(Chat::White, "Native HPFIX payload refreshed.");
		return;
	}

	if (!strcasecmp(sep->arg[1], "status")) {
		const std::string ready_status = c->IsNativeHpFixReady() ? "ready" : "not ready";
		c->Message(
			Chat::White,
			fmt::format(
				"Native HPFIX is {}. Server HP: {} / {}.",
				ready_status,
				c->GetHP(),
				c->GetMaxHP()
			).c_str()
		);
		return;
	}

	if (!strcasecmp(sep->arg[1], "window")) {
		c->SetNativeHpFixReady(true);
		c->Message(Chat::Yellow, "HPFIX|window|show");
		c->SendNativeHpFixUpdate(true);
		return;
	}

	if (!strcasecmp(sep->arg[1], "off")) {
		c->SetNativeHpFixReady(false);
		c->Message(Chat::White, "Native HPFIX disabled for this zone session.");
		return;
	}

	if (!strcasecmp(sep->arg[1], "items")) {
		if (!HpFixCanUseGmTools(c)) {
			c->Message(Chat::White, "Insufficient status to summon HPFIX test items.");
			return;
		}

		HpFixSummonItem(c, HPFIX_CHARM_ITEM_ID);
		HpFixSummonItem(c, HPFIX_CHEST_ITEM_ID);
		HpFixSummonItem(c, HPFIX_WAIST_ITEM_ID);
		return;
	}

	HpFixShowUsage(c);
}
