#pragma once

#include "../common/types.h"

#include <string>

struct THJFakeBazaarActionResult {
	bool        success = false;
	bool        skipped = false;
	std::string message;
	int         cleared_traders = 0;
	int         cleared_buyers = 0;
	int         cleared_buyer_lines = 0;
	int         cleared_buyer_trade_items = 0;
	int         inserted_traders = 0;
	int         inserted_buy_lines = 0;
	uint32      seller_count = 0;
	uint32      buyer_count = 0;
	uint32      seed_zone_instance_id = 0;
	uint32      min_expansion = 0;
	uint32      max_expansion = 0;
	uint32      drops_per_zone = 0;
};

THJFakeBazaarActionResult THJFakeBazaarClearRows();
THJFakeBazaarActionResult THJFakeBazaarSeedRows(uint32 seed_zone_instance_id = 0, const std::string &reason = "manual");
THJFakeBazaarActionResult THJFakeBazaarRefreshRows(bool force = false, const std::string &reason = "manual");
void THJFakeBazaarProcessAutoRefresh();
