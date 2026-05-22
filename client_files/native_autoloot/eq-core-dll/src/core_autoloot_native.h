#ifndef CORE_AUTOLOOT_NATIVE_H
#define CORE_AUTOLOOT_NATIVE_H

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static void AoTAutoLootTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) {
		sprintf_s(path, "%s\\aot_autoloot_native.log", gszEQPath);
	}
	else {
		strcpy_s(path, "aot_autoloot_native.log");
	}

	FILE* file = nullptr;
	if (fopen_s(&file, path, "a") || !file) {
		return;
	}

	SYSTEMTIME now;
	GetLocalTime(&now);
	fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] ",
		now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);

	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);

	fprintf(file, "\n");
	fclose(file);
}

struct AoTAutoLootRow
{
	int entry_id = 0;
	int item_id = 0;
	int icon_id = 0;
	int qty = 1;
	bool shared = false;
	bool locked = false;
	bool nodrop = false;
	std::string item = "Item";
	std::string source = "corpse";
	std::string state = "waiting";
	std::string rule = "-";
};

static void AoTAutoLootSendCommand(const char* command);
static void AoTAutoLootUpdateWindow();
static const char* AoTAutoLootToggleEnabledCommand();
static void AoTAutoLootShowRulesWindow();

class AoTAutoLootWnd : public CCustomWnd
{
public:
	AoTAutoLootWnd() : CCustomWnd((char*)"AoTAutoLootWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(AoTAutoLootWnd);

		PersonalLabel = GetChildItem("AALW_PersonalLabel");
		SetAllLabel = GetChildItem("AALW_SetAllLabel");
		SharedLabel = GetChildItem("AALW_SharedLabel");
		PersonalList = (CListWnd*)GetChildItem("AALW_PersonalList");
		SharedList = (CListWnd*)GetChildItem("AALW_SharedList");
		StatusLabel = GetChildItem("AALW_StatusLabel");
		MasterLabel = GetChildItem("AALW_MasterLabel");
		RuleSummaryLabel = GetChildItem("AALW_RuleSummaryLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("AALW_RefreshButton");
		NearbyButton = (CButtonWnd*)GetChildItem("AALW_NearbyButton");
		EditFiltersButton = (CButtonWnd*)GetChildItem("AALW_EditFiltersButton");
		LootAllButton = (CButtonWnd*)GetChildItem("AALW_LootAllButton");
		LeaveAllButton = (CButtonWnd*)GetChildItem("AALW_LeaveAllButton");
		LootButton = (CButtonWnd*)GetChildItem("AALW_LootButton");
		LeaveButton = (CButtonWnd*)GetChildItem("AALW_LeaveButton");
		AlwaysButton = (CButtonWnd*)GetChildItem("AALW_AlwaysButton");
		NeverButton = (CButtonWnd*)GetChildItem("AALW_NeverButton");
		ApplyFiltersCheck = (CButtonWnd*)GetChildItem("AALW_ApplyFiltersCheck");

		Layout();
		SetStatus("Waiting for AutoLoot snapshot...");
		RefreshRows();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)PersonalList || pWnd == (CXWnd*)SharedList) {
				ActiveList = (CListWnd*)pWnd;
				return 1;
			}

			if (pWnd == (CXWnd*)RefreshButton) {
				AoTAutoLootSendCommand("/say #autoloot native status");
				SetStatus("Refreshing AutoLoot...");
				return 1;
			}

			if (pWnd == (CXWnd*)NearbyButton) {
				AoTAutoLootSendCommand("/say #autoloot nearby 75");
				SetStatus("Scanning nearby corpses...");
				return 1;
			}

			if (pWnd == (CXWnd*)EditFiltersButton) {
				AoTAutoLootShowRulesWindow();
				SetStatus("Opened AutoLoot filters.");
				return 1;
			}

			if (pWnd == (CXWnd*)LootAllButton) {
				AoTAutoLootSendCommand("/say #autoloot personal lootall");
				SetStatus("Requested Loot All.");
				return 1;
			}

			if (pWnd == (CXWnd*)LeaveAllButton) {
				AoTAutoLootSendCommand("/say #autoloot personal leaveall");
				SetStatus("Requested Leave All.");
				return 1;
			}

			if (pWnd == (CXWnd*)ApplyFiltersCheck) {
				AoTAutoLootSendCommand(AoTAutoLootToggleEnabledCommand());
				SetStatus("Toggled AutoLoot.");
				return 1;
			}

			AoTAutoLootRow* row = GetSelectedRow();
			if (!row) {
				SetStatus("Select a real loot row first.");
				return 1;
			}

			if (pWnd == (CXWnd*)LootButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d loot", row->entry_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested loot.");
				return 1;
			}

			if (pWnd == (CXWnd*)LeaveButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d leave", row->entry_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested leave.");
				return 1;
			}

			if (pWnd == (CXWnd*)AlwaysButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d alwaysloot", row->entry_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested always loot.");
				return 1;
			}

			if (pWnd == (CXWnd*)NeverButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d never", row->entry_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested never loot.");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout();
	void SetStatus(const char* text)
	{
		if (StatusLabel) {
			CXStr value(text);
			StatusLabel->SetWindowTextA(value);
		}
	}

	void RefreshRows();

private:
	AoTAutoLootRow* GetSelectedRow();
	void RefreshList(CListWnd* list, bool shared);
	void SetLabel(CXWnd* label, const char* text);

	CXWnd* PersonalLabel = nullptr;
	CXWnd* SetAllLabel = nullptr;
	CXWnd* SharedLabel = nullptr;
	CListWnd* PersonalList = nullptr;
	CListWnd* SharedList = nullptr;
	CListWnd* ActiveList = nullptr;
	CXWnd* StatusLabel = nullptr;
	CXWnd* MasterLabel = nullptr;
	CXWnd* RuleSummaryLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* NearbyButton = nullptr;
	CButtonWnd* EditFiltersButton = nullptr;
	CButtonWnd* LootAllButton = nullptr;
	CButtonWnd* LeaveAllButton = nullptr;
	CButtonWnd* LootButton = nullptr;
	CButtonWnd* LeaveButton = nullptr;
	CButtonWnd* AlwaysButton = nullptr;
	CButtonWnd* NeverButton = nullptr;
	CButtonWnd* ApplyFiltersCheck = nullptr;
	int LastLayoutWidth = 0;
	int LastLayoutHeight = 0;
};

static AoTAutoLootWnd* gAoTAutoLootWnd = nullptr;
static std::vector<AoTAutoLootRow> gAoTAutoLootRows;
static bool gAoTAutoLootHooksInstalled = false;
static bool gAoTAutoLootChatHookInstalled = false;
static bool gAoTAutoLootRequestedInitialStatus = false;
static bool gAoTAutoLootEnabled = false;
static bool gAoTAutoLootGrouped = false;
static bool gAoTAutoLootLeader = false;
static int gAoTAutoLootInGamePulses = 0;
static int gAoTAutoLootKeepCount = 0;
static int gAoTAutoLootIgnoreCount = 0;
static std::string gAoTAutoLootGroupMode = "solo";
static std::string gAoTAutoLootAssigned = "none";

struct AoTAutoLootRuleRow
{
	int item_id = 0;
	int icon_id = 0;
	std::string item = "Item";
	std::string rule = "include";
};

class AoTAutoLootRulesWnd : public CCustomWnd
{
public:
	AoTAutoLootRulesWnd() : CCustomWnd((char*)"AoTAutoLootRulesWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(AoTAutoLootRulesWnd);

		SummaryLabel = GetChildItem("AALR_SummaryLabel");
		StatusLabel = GetChildItem("AALR_StatusLabel");
		RuleList = (CListWnd*)GetChildItem("AALR_RuleList");
		RefreshButton = (CButtonWnd*)GetChildItem("AALR_RefreshButton");
		KeepButton = (CButtonWnd*)GetChildItem("AALR_KeepButton");
		IgnoreButton = (CButtonWnd*)GetChildItem("AALR_IgnoreButton");
		UnsetButton = (CButtonWnd*)GetChildItem("AALR_UnsetButton");

		Layout();
		RefreshRows();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)RefreshButton) {
				AoTAutoLootSendCommand("/say #lootfilter native list both");
				SetStatus("Refreshing filters...");
				return 1;
			}

			AoTAutoLootRuleRow* row = GetSelectedRule();
			if (!row) {
				SetStatus("Select a rule first.");
				return 1;
			}

			char command[128];
			if (pWnd == (CXWnd*)KeepButton) {
				sprintf_s(command, "/say #lootfilter keep %d", row->item_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested Keep rule.");
				return 1;
			}

			if (pWnd == (CXWnd*)IgnoreButton) {
				sprintf_s(command, "/say #lootfilter ignore %d", row->item_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested Never rule.");
				return 1;
			}

			if (pWnd == (CXWnd*)UnsetButton) {
				sprintf_s(command, "/say #lootfilter unset %d", row->item_id);
				AoTAutoLootSendCommand(command);
				SetStatus("Requested rule removal.");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout();
	void RefreshRows();
	void SetStatus(const char* text)
	{
		if (StatusLabel) {
			CXStr value(text);
			StatusLabel->SetWindowTextA(value);
		}
	}

private:
	AoTAutoLootRuleRow* GetSelectedRule();
	void SetLabel(CXWnd* label, const char* text);

	CXWnd* SummaryLabel = nullptr;
	CXWnd* StatusLabel = nullptr;
	CListWnd* RuleList = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* KeepButton = nullptr;
	CButtonWnd* IgnoreButton = nullptr;
	CButtonWnd* UnsetButton = nullptr;
	int LastLayoutWidth = 0;
	int LastLayoutHeight = 0;
};

static AoTAutoLootRulesWnd* gAoTAutoLootRulesWnd = nullptr;
static std::vector<AoTAutoLootRuleRow> gAoTAutoLootRuleRows;

static const char* AoTAutoLootToggleEnabledCommand()
{
	return gAoTAutoLootEnabled ? "/say #autoloot off" : "/say #autoloot on";
}

static bool AoTStartsWith(const char* value, const char* prefix)
{
	return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
}

static std::string AoTGetPairValue(const std::string& payload, const char* key)
{
	const std::string prefix = std::string(key) + "=";
	size_t pos = 0;

	while (pos < payload.size()) {
		const size_t end = payload.find('|', pos);
		const size_t count = end == std::string::npos ? std::string::npos : end - pos;
		const std::string part = payload.substr(pos, count);

		if (part.compare(0, prefix.size(), prefix) == 0) {
			return part.substr(prefix.size());
		}

		if (end == std::string::npos) {
			break;
		}

		pos = end + 1;
	}

	return std::string();
}

static int AoTToInt(const std::string& value, int fallback = 0)
{
	if (value.empty()) {
		return fallback;
	}

	return atoi(value.c_str());
}

static bool AoTToBool(const std::string& value)
{
	return AoTToInt(value) != 0 || value == "true" || value == "on";
}

static bool AoTIsKeepRule(const std::string& rule)
{
	return rule == "include" || rule == "keep" || rule == "always" || rule == "loot";
}

static bool AoTIsNeverRule(const std::string& rule)
{
	return rule == "exclude" || rule == "ignore" || rule == "never" || rule == "skip";
}

static const char* AoTShortRule(const std::string& rule)
{
	if (AoTIsKeepRule(rule)) {
		return "X";
	}

	if (AoTIsNeverRule(rule)) {
		return "X";
	}

	return "";
}

static const char* AoTDisplayRule(const std::string& rule)
{
	if (AoTIsNeverRule(rule)) {
		return "Never";
	}

	if (AoTIsKeepRule(rule)) {
		return "Keep";
	}

	return "-";
}

void AoTAutoLootRulesWnd::SetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text);
		label->SetWindowTextA(value);
	}
}

void AoTAutoLootRulesWnd::Layout()
{
	// Filters resize is handled by SIDL AutoStretch anchors. Avoid manually
	// moving children during construction; this client build can fault there.
}

void AoTAutoLootRulesWnd::RefreshRows()
{
	if (!RuleList) {
		return;
	}

	RuleList->DeleteAll();

	char summary[96];
	sprintf_s(summary, "Rules %d keep / %d never", gAoTAutoLootKeepCount, gAoTAutoLootIgnoreCount);
	SetLabel(SummaryLabel, summary);

	if (gAoTAutoLootRuleRows.empty()) {
		CXStr rule("-");
		const int row = RuleList->AddString(rule, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr item("No filters loaded.");
		CXStr item_id("-");
		RuleList->SetItemText(row, 1, &item);
		RuleList->SetItemText(row, 2, &item_id);
		return;
	}

	for (const AoTAutoLootRuleRow& entry : gAoTAutoLootRuleRows) {
		CXStr rule(AoTDisplayRule(entry.rule));
		const int row = RuleList->AddString(rule, AoTIsNeverRule(entry.rule) ? 0xFFFF8080 : 0xFF66FF66, (uint32_t)entry.item_id, nullptr, nullptr);

		char item_id[32];
		sprintf_s(item_id, "%d", entry.item_id);
		CXStr item(entry.item.c_str());
		CXStr id(item_id);
		RuleList->SetItemText(row, 1, &item);
		RuleList->SetItemText(row, 2, &id);
	}
}

AoTAutoLootRuleRow* AoTAutoLootRulesWnd::GetSelectedRule()
{
	if (!RuleList) {
		return nullptr;
	}

	const int selected = RuleList->GetCurSel();
	if (selected < 0) {
		return nullptr;
	}

	const int item_id = (int)RuleList->GetItemData(selected);
	if (item_id <= 0) {
		return nullptr;
	}

	for (AoTAutoLootRuleRow& row : gAoTAutoLootRuleRows) {
		if (row.item_id == item_id) {
			return &row;
		}
	}

	return nullptr;
}

static void AoTAutoLootShowRulesWindow()
{
	if (!gAoTAutoLootRulesWnd) {
		AoTAutoLootTrace("creating rules window");
		gAoTAutoLootRulesWnd = new AoTAutoLootRulesWnd();
	}

	gAoTAutoLootRulesWnd->RefreshRows();
	gAoTAutoLootRulesWnd->pXWnd()->Show(1, 1);
	gAoTAutoLootRulesWnd->SetStatus("Refreshing filters...");
	AoTAutoLootSendCommand("/say #lootfilter native list both");
}

void AoTAutoLootWnd::SetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text);
		label->SetWindowTextA(value);
	}
}

void AoTAutoLootWnd::Layout()
{
	// Main-window resize is handled by SIDL AutoStretch anchors. Moving many
	// child controls manually during construction can fault this client build.
}

void AoTAutoLootWnd::RefreshList(CListWnd* list, bool shared)
{
	if (!list) {
		return;
	}

	list->DeleteAll();
	int visible = 0;

	for (const AoTAutoLootRow& entry : gAoTAutoLootRows) {
		if (entry.shared != shared) {
			continue;
		}

		++visible;
		const COLORREF row_color = entry.locked ? 0xFFFF8080 : 0xFFFFFFFF;
		char item_text[256];
		if (entry.qty > 1) {
			sprintf_s(item_text, "%s x%d", entry.item.c_str(), entry.qty);
		}
		else {
			sprintf_s(item_text, "%s", entry.item.c_str());
		}

		CXStr item(item_text);
		const int row = list->AddString(item, row_color, (uint32_t)entry.entry_id, nullptr, nullptr);

		if (!shared) {
			char qty_text[16];
			sprintf_s(qty_text, "%d", entry.qty);
			CXStr qty(qty_text);
			CXStr source(entry.source.c_str());
			CXStr rule(AoTDisplayRule(entry.rule));
			CXStr status(entry.locked ? "Locked" : entry.state.c_str());
			list->SetItemText(row, 1, &qty);
			list->SetItemText(row, 2, &source);
			list->SetItemText(row, 3, &rule);
			list->SetItemText(row, 4, &status);
			list->SetItemColor(row, 3, AoTIsNeverRule(entry.rule) ? 0xFFFF8080 : 0xFF66FF66);
			list->SetItemColor(row, 4, entry.locked ? 0xFFFF8080 : 0xFFFFFFFF);
		}
		else {
			char qty_text[16];
			sprintf_s(qty_text, "%d", entry.qty);
			CXStr qty(qty_text);
			CXStr source(entry.source.c_str());
			CXStr status(entry.locked ? "Locked" : entry.state.c_str());
			CXStr rule(AoTDisplayRule(entry.rule));
			CXStr nd(entry.state == "need" || entry.state == "alwaysneed" ? "X" : "");
			CXStr gd(entry.state == "greed" || entry.state == "alwaysgreed" ? "X" : "");
			CXStr no(entry.state == "no" ? "X" : "");
			CXStr an(entry.state == "alwaysneed" ? "X" : AoTShortRule(entry.rule));
			CXStr ag(entry.state == "alwaysgreed" ? "X" : "");
			CXStr nv(AoTIsNeverRule(entry.rule) ? "X" : "");
			list->SetItemText(row, 1, &qty);
			list->SetItemText(row, 2, &source);
			list->SetItemText(row, 3, &status);
			list->SetItemText(row, 4, &rule);
			list->SetItemText(row, 5, &nd);
			list->SetItemText(row, 6, &gd);
			list->SetItemText(row, 7, &no);
			list->SetItemText(row, 8, &an);
			list->SetItemText(row, 9, &ag);
			list->SetItemText(row, 10, &nv);
			list->SetItemColor(row, 4, AoTIsNeverRule(entry.rule) ? 0xFFFF8080 : 0xFF66FF66);
			list->SetItemColor(row, 5, 0xFF66FF66);
			list->SetItemColor(row, 6, 0xFFFFFF80);
			list->SetItemColor(row, 7, 0xFFFF8080);
			list->SetItemColor(row, 10, 0xFFFF8080);
		}
	}

	if (visible) {
		return;
	}

	CXStr dash("-");
	const int row = list->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
	if (!shared) {
		CXStr empty("No personal loot.");
		list->SetItemText(row, 1, &dash);
		list->SetItemText(row, 2, &empty);
		list->SetItemText(row, 3, &dash);
		list->SetItemText(row, 4, &dash);
	}
	else {
		CXStr empty(gAoTAutoLootGrouped ? "No shared loot." : "Not grouped.");
		list->SetItemText(row, 1, &dash);
		list->SetItemText(row, 2, &empty);
		list->SetItemText(row, 3, &dash);
		list->SetItemText(row, 4, &dash);
		list->SetItemText(row, 5, &dash);
		list->SetItemText(row, 6, &dash);
		list->SetItemText(row, 7, &dash);
		list->SetItemText(row, 8, &dash);
		list->SetItemText(row, 9, &dash);
		list->SetItemText(row, 10, &empty);
	}
}

void AoTAutoLootWnd::RefreshRows()
{
	RefreshList(PersonalList, false);
	RefreshList(SharedList, true);

	char rules[96];
	sprintf_s(rules, "Rules %d keep / %d never", gAoTAutoLootKeepCount, gAoTAutoLootIgnoreCount);
	SetLabel(RuleSummaryLabel, rules);

	char master[128];
	sprintf_s(master, "Group: %s  Assigned: %s", gAoTAutoLootGroupMode.c_str(), gAoTAutoLootAssigned.c_str());
	SetLabel(MasterLabel, master);

	if (ApplyFiltersCheck) {
		CXStr value(gAoTAutoLootEnabled ? "AutoLoot On" : "AutoLoot Off");
		((CXWnd*)ApplyFiltersCheck)->SetWindowTextA(value);
	}
}

AoTAutoLootRow* AoTAutoLootWnd::GetSelectedRow()
{
	CListWnd* list = ActiveList ? ActiveList : PersonalList;
	if (!list) {
		return nullptr;
	}

	int selected = list->GetCurSel();
	if (selected < 0 && list != SharedList && SharedList) {
		list = SharedList;
		selected = list->GetCurSel();
	}

	if (selected < 0) {
		return nullptr;
	}

	const int entry_id = (int)list->GetItemData(selected);
	if (entry_id <= 0) {
		return nullptr;
	}

	for (AoTAutoLootRow& row : gAoTAutoLootRows) {
		if (row.entry_id == entry_id) {
			return &row;
		}
	}

	return nullptr;
}

static void AoTAutoLootSendCommand(const char* command)
{
	if (!command || !command[0] || !pEverQuest || !pLocalPlayer) {
		return;
	}

	AoTAutoLootTrace("send command: %s", command);
	char buffer[256];
	strcpy_s(buffer, command);
	pEverQuest->InterpretCmd((EQPlayer*)pLocalPlayer, buffer);
}

static void AoTAutoLootUpdateWindow()
{
	if (!gAoTAutoLootWnd) {
		return;
	}

	int personal = 0;
	int shared = 0;
	for (const AoTAutoLootRow& row : gAoTAutoLootRows) {
		if (row.shared) {
			++shared;
		}
		else {
			++personal;
		}
	}

	gAoTAutoLootWnd->RefreshRows();
	char status[128];
	sprintf_s(status, "Personal %d / Shared %d", personal, shared);
	gAoTAutoLootWnd->SetStatus(status);
}

static bool AoTAutoLootParseTransport(const char* message)
{
	if (!message || !message[0]) {
		return false;
	}

	if (AoTStartsWith(message, "AUTOLOOT|snapshot|begin") || AoTStartsWith(message, "AUTOLOOT|clear|loot")) {
		gAoTAutoLootRows.clear();
		AoTAutoLootUpdateWindow();
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|snapshot|end")) {
		AoTAutoLootUpdateWindow();
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|window|show")) {
		if (gAoTAutoLootWnd) {
			gAoTAutoLootWnd->pXWnd()->Show(1, 1);
			gAoTAutoLootWnd->SetStatus("AutoLoot window reopened.");
		}
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|entry|") || AoTStartsWith(message, "AUTOLOOT|loot|")) {
		const char* payload_start = strchr(message + 9, '|');
		if (!payload_start) {
			return true;
		}

		const std::string payload(payload_start + 1);
		AoTAutoLootRow row;
		row.shared = AoTGetPairValue(payload, "scope") == "shared";
		row.entry_id = AoTToInt(AoTGetPairValue(payload, "id"));
		row.item_id = AoTToInt(AoTGetPairValue(payload, "item_id"), AoTToInt(AoTGetPairValue(payload, "item")));
		row.icon_id = AoTToInt(AoTGetPairValue(payload, "icon"));
		row.qty = AoTToInt(AoTGetPairValue(payload, "qty"), AoTToInt(AoTGetPairValue(payload, "quantity"), 1));
		if (row.qty < 1) {
			row.qty = 1;
		}
		row.locked = AoTToBool(AoTGetPairValue(payload, "locked"));
		row.nodrop = AoTToBool(AoTGetPairValue(payload, "nodrop"));
		row.item = AoTGetPairValue(payload, "name");
		if (row.item.empty()) {
			row.item = AoTGetPairValue(payload, "item_name");
		}
		if (row.item.empty()) {
			char name[64];
			sprintf_s(name, "Item %d", row.item_id);
			row.item = name;
		}
		row.source = AoTGetPairValue(payload, "source");
		if (row.source.empty()) {
			row.source = AoTGetPairValue(payload, "corpse");
		}
		if (row.source.empty()) {
			row.source = "corpse";
		}
		row.state = AoTGetPairValue(payload, "state");
		if (row.state.empty()) {
			row.state = AoTGetPairValue(payload, "status");
		}
		if (row.state.empty()) {
			row.state = "waiting";
		}
		row.rule = AoTGetPairValue(payload, "rule");
		if (row.rule.empty()) {
			row.rule = AoTGetPairValue(payload, "filter");
		}
		if (row.rule.empty()) {
			row.rule = "-";
		}

		if (row.entry_id > 0) {
			gAoTAutoLootRows.push_back(row);
			AoTAutoLootUpdateWindow();
		}
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|status|")) {
		const std::string payload(message + strlen("AUTOLOOT|status|"));
		gAoTAutoLootEnabled = AoTToBool(AoTGetPairValue(payload, "enabled"));
		gAoTAutoLootGrouped = AoTToBool(AoTGetPairValue(payload, "grouped"));
		gAoTAutoLootLeader = AoTToBool(AoTGetPairValue(payload, "leader"));
		gAoTAutoLootKeepCount = AoTToInt(AoTGetPairValue(payload, "include"));
		gAoTAutoLootIgnoreCount = AoTToInt(AoTGetPairValue(payload, "exclude"));
		gAoTAutoLootGroupMode = AoTGetPairValue(payload, "group_mode");
		if (gAoTAutoLootGroupMode.empty()) {
			gAoTAutoLootGroupMode = gAoTAutoLootGrouped ? "group" : "solo";
		}
		gAoTAutoLootAssigned = AoTGetPairValue(payload, "assigned");
		if (gAoTAutoLootAssigned.empty()) {
			gAoTAutoLootAssigned = "none";
		}
		AoTAutoLootUpdateWindow();
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|filters|begin")) {
		gAoTAutoLootRuleRows.clear();
		if (gAoTAutoLootRulesWnd) {
			gAoTAutoLootRulesWnd->RefreshRows();
			gAoTAutoLootRulesWnd->SetStatus("Loading filters...");
		}
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|filter|")) {
		const std::string payload(message + strlen("AUTOLOOT|filter|"));
		AoTAutoLootRuleRow row;
		row.rule = AoTGetPairValue(payload, "mode");
		row.item_id = AoTToInt(AoTGetPairValue(payload, "item_id"));
		row.icon_id = AoTToInt(AoTGetPairValue(payload, "icon"));
		row.item = AoTGetPairValue(payload, "name");
		if (row.item.empty()) {
			char name[64];
			sprintf_s(name, "Item %d", row.item_id);
			row.item = name;
		}

		if (row.item_id > 0) {
			gAoTAutoLootRuleRows.push_back(row);
		}
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|filters|end")) {
		if (gAoTAutoLootRulesWnd) {
			gAoTAutoLootRulesWnd->RefreshRows();
			gAoTAutoLootRulesWnd->SetStatus("Filters refreshed.");
		}
		return true;
	}

	if (AoTStartsWith(message, "AUTOLOOT|filters|")) {
		return true;
	}

	if (strstr(message, "You say, '#autoloot") || strstr(message, "You say, '#lootfilter")) {
		return true;
	}

	return false;
}

class AoTAutoLootChatHook
{
public:
	VOID Trampoline(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst);
	VOID Detour(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst)
	{
		if (AoTAutoLootParseTransport(szMsg)) {
			return;
		}

		Trampoline(szMsg, dwColor, EqLog, dopercentsubst);
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID AoTAutoLootChatHook::Trampoline(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst));

static void AoTAutoLootInstallChatHook()
{
	if (gAoTAutoLootChatHookInstalled) {
		return;
	}

	AoTAutoLootTrace("installing chat hook");
	EzDetour(CEverQuest__dsp_chat, &AoTAutoLootChatHook::Detour, &AoTAutoLootChatHook::Trampoline);
	gAoTAutoLootChatHookInstalled = true;
}

static void AoTAutoLootPulse()
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	const DWORD state = GetGameState();
	if (state != GAMESTATE_INGAME) {
		gAoTAutoLootInGamePulses = 0;
		return;
	}

	if (!pEverQuest || !pLocalPlayer || !GetCharInfo2()) {
		gAoTAutoLootInGamePulses = 0;
		return;
	}

	if (gAoTAutoLootInGamePulses < 120) {
		++gAoTAutoLootInGamePulses;
		return;
	}

	if (!gAoTAutoLootWnd) {
		AoTAutoLootTrace("creating window");
		gAoTAutoLootWnd = new AoTAutoLootWnd();
		gAoTAutoLootWnd->RefreshRows();
		AoTAutoLootTrace("window created");
	}

	AoTAutoLootInstallChatHook();

	if (gAoTAutoLootWnd) {
		gAoTAutoLootWnd->Layout();
	}

	if (gAoTAutoLootRulesWnd) {
		gAoTAutoLootRulesWnd->Layout();
	}

	if (!gAoTAutoLootRequestedInitialStatus) {
		gAoTAutoLootRequestedInitialStatus = true;
		AoTAutoLootSendCommand("/say #autoloot native status");
	}
}

BOOL AoTAutoLoot_ProcessGameEvents_Trampoline(VOID);
BOOL AoTAutoLoot_ProcessGameEvents_Detour(VOID)
{
	AoTAutoLootPulse();
	return AoTAutoLoot_ProcessGameEvents_Trampoline();
}
DETOUR_TRAMPOLINE_EMPTY(BOOL AoTAutoLoot_ProcessGameEvents_Trampoline(VOID));

static void InitAutoLootNative()
{
	if (gAoTAutoLootHooksInstalled) {
		return;
	}

	AoTAutoLootTrace("installing pulse hook");
	gAoTAutoLootHooksInstalled = true;
	EzDetour(ProcessGameEvents, AoTAutoLoot_ProcessGameEvents_Detour, AoTAutoLoot_ProcessGameEvents_Trampoline);
}

static void ShutdownAutoLootNative()
{
	AoTAutoLootTrace("shutdown");

	if (gAoTAutoLootWnd) {
		delete gAoTAutoLootWnd;
		gAoTAutoLootWnd = nullptr;
	}

	if (gAoTAutoLootRulesWnd) {
		delete gAoTAutoLootRulesWnd;
		gAoTAutoLootRulesWnd = nullptr;
	}

	if (gAoTAutoLootChatHookInstalled) {
		RemoveDetour(CEverQuest__dsp_chat);
		gAoTAutoLootChatHookInstalled = false;
	}

	if (gAoTAutoLootHooksInstalled) {
		RemoveDetour((DWORD)ProcessGameEvents);
		gAoTAutoLootHooksInstalled = false;
	}

	gAoTAutoLootRequestedInitialStatus = false;
	gAoTAutoLootInGamePulses = 0;
}

#endif
