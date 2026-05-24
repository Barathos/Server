#ifndef CORE_AUTOLOOT_NATIVE_H
#define CORE_AUTOLOOT_NATIVE_H

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static void NativeAutoLootTrace(const char* format, ...)
{
	char path[MAX_PATH];
	if (gszEQPath[0]) {
		sprintf_s(path, "%s\\native_autoloot.log", gszEQPath);
	}
	else {
		strcpy_s(path, "native_autoloot.log");
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

struct NativeAutoLootRow
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

static void NativeAutoLootSendCommand(const char* command);
static void NativeAutoLootUpdateWindow();
static const char* NativeAutoLootToggleEnabledCommand();
static void NativeAutoLootShowRulesWindow();
static void NativeAutoLootMaybeSendInitialRequests();
static void NativeSpellForgeShowWindow(const std::string& payload);
static void NativeItemForgeShowWindow(const std::string& payload);
static void NativeAchievementEnsureWindow(bool show);
static bool NativeAchievementParseTransport(const char* message);

class NativeAutoLootWnd : public CCustomWnd
{
public:
	NativeAutoLootWnd() : CCustomWnd((char*)"NativeAutoLootWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAutoLootWnd);

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
				NativeAutoLootSendCommand("/say #autoloot native status");
				SetStatus("Refreshing AutoLoot...");
				return 1;
			}

			if (pWnd == (CXWnd*)NearbyButton) {
				NativeAutoLootSendCommand("/say #autoloot nearby 75");
				SetStatus("Scanning nearby corpses...");
				return 1;
			}

			if (pWnd == (CXWnd*)EditFiltersButton) {
				NativeAutoLootShowRulesWindow();
				SetStatus("Opened AutoLoot filters.");
				return 1;
			}

			if (pWnd == (CXWnd*)LootAllButton) {
				NativeAutoLootSendCommand("/say #autoloot personal lootall");
				SetStatus("Requested Loot All.");
				return 1;
			}

			if (pWnd == (CXWnd*)LeaveAllButton) {
				NativeAutoLootSendCommand("/say #autoloot personal leaveall");
				SetStatus("Requested Leave All.");
				return 1;
			}

			if (pWnd == (CXWnd*)ApplyFiltersCheck) {
				NativeAutoLootSendCommand(NativeAutoLootToggleEnabledCommand());
				SetStatus("Toggled AutoLoot.");
				return 1;
			}

			NativeAutoLootRow* row = GetSelectedRow();
			if (!row) {
				SetStatus("Select a real loot row first.");
				return 1;
			}

			if (pWnd == (CXWnd*)LootButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d loot", row->entry_id);
				NativeAutoLootSendCommand(command);
				SetStatus("Requested loot.");
				return 1;
			}

			if (pWnd == (CXWnd*)LeaveButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d leave", row->entry_id);
				NativeAutoLootSendCommand(command);
				SetStatus("Requested leave.");
				return 1;
			}

			if (pWnd == (CXWnd*)AlwaysButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d alwaysloot", row->entry_id);
				NativeAutoLootSendCommand(command);
				SetStatus("Requested always loot.");
				return 1;
			}

			if (pWnd == (CXWnd*)NeverButton) {
				char command[128];
				sprintf_s(command, "/say #autoloot action %d never", row->entry_id);
				NativeAutoLootSendCommand(command);
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
	NativeAutoLootRow* GetSelectedRow();
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

static NativeAutoLootWnd* gNativeAutoLootWnd = nullptr;
static std::vector<NativeAutoLootRow> gNativeAutoLootRows;
static bool gNativeAutoLootHooksInstalled = false;
static bool gNativeAutoLootChatHookInstalled = false;
static bool gNativeAutoLootCommandHookInstalled = false;
static bool gNativeAutoLootPulseHookInstalled = false;
static bool gNativeAutoLootRequestedInitialStatus = false;
static bool gNativeAutoLootPulseHookEnabled = false;
static bool gNativeAutoLootEnabled = false;
static bool gNativeAutoLootGrouped = false;
static bool gNativeAutoLootLeader = false;
static int gNativeAutoLootInGamePulses = 0;
static int gNativeAutoLootKeepCount = 0;
static int gNativeAutoLootIgnoreCount = 0;
static bool gNativeLiveSpellSentReady = false;
static bool gNativeAutoLootPulseFaulted = false;
static std::string gNativeAutoLootGroupMode = "solo";
static std::string gNativeAutoLootAssigned = "none";

struct NativeAutoLootRuleRow
{
	int item_id = 0;
	int icon_id = 0;
	std::string item = "Item";
	std::string rule = "include";
};

class NativeAutoLootRulesWnd : public CCustomWnd
{
public:
	NativeAutoLootRulesWnd() : CCustomWnd((char*)"NativeAutoLootRulesWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAutoLootRulesWnd);

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
				NativeAutoLootSendCommand("/say #lootfilter native list both");
				SetStatus("Refreshing filters...");
				return 1;
			}

			NativeAutoLootRuleRow* row = GetSelectedRule();
			if (!row) {
				SetStatus("Select a rule first.");
				return 1;
			}

			char command[128];
			if (pWnd == (CXWnd*)KeepButton) {
				sprintf_s(command, "/say #lootfilter keep %d", row->item_id);
				NativeAutoLootSendCommand(command);
				SetStatus("Requested Keep rule.");
				return 1;
			}

			if (pWnd == (CXWnd*)IgnoreButton) {
				sprintf_s(command, "/say #lootfilter ignore %d", row->item_id);
				NativeAutoLootSendCommand(command);
				SetStatus("Requested Never rule.");
				return 1;
			}

			if (pWnd == (CXWnd*)UnsetButton) {
				sprintf_s(command, "/say #lootfilter unset %d", row->item_id);
				NativeAutoLootSendCommand(command);
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
	NativeAutoLootRuleRow* GetSelectedRule();
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

static NativeAutoLootRulesWnd* gNativeAutoLootRulesWnd = nullptr;
static std::vector<NativeAutoLootRuleRow> gNativeAutoLootRuleRows;

static const char* NativeAutoLootToggleEnabledCommand()
{
	return gNativeAutoLootEnabled ? "/say #autoloot off" : "/say #autoloot on";
}

static bool NativeStartsWith(const char* value, const char* prefix)
{
	return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
}

static std::string NativeGetPairValue(const std::string& payload, const char* key)
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

static int NativeToInt(const std::string& value, int fallback = 0)
{
	if (value.empty()) {
		return fallback;
	}

	return atoi(value.c_str());
}

static bool NativeToBool(const std::string& value)
{
	return NativeToInt(value) != 0 || value == "true" || value == "on";
}

static float NativeToFloat(const std::string& value, float fallback = 0.0f)
{
	if (value.empty()) {
		return fallback;
	}

	return static_cast<float>(atof(value.c_str()));
}

template <size_t Size>
static void NativeCopyText(char (&destination)[Size], const std::string& source)
{
	memset(destination, 0, Size);
	strncpy_s(destination, source.c_str(), Size - 1);
}

static bool NativeIsKeepRule(const std::string& rule)
{
	return rule == "include" || rule == "keep" || rule == "always" || rule == "loot";
}

static bool NativeIsNeverRule(const std::string& rule)
{
	return rule == "exclude" || rule == "ignore" || rule == "never" || rule == "skip";
}

static const char* NativeShortRule(const std::string& rule)
{
	if (NativeIsKeepRule(rule)) {
		return "X";
	}

	if (NativeIsNeverRule(rule)) {
		return "X";
	}

	return "";
}

static const char* NativeDisplayRule(const std::string& rule)
{
	if (NativeIsNeverRule(rule)) {
		return "Never";
	}

	if (NativeIsKeepRule(rule)) {
		return "Keep";
	}

	return "-";
}

struct NativeAchievementCategoryRow
{
	int id = 0;
	int parent_id = 0;
	int total = 0;
	int completed = 0;
	int points = 0;
	std::string name = "Category";
};

struct NativeAchievementRow
{
	int id = 0;
	int category_id = 0;
	int points = 0;
	bool completed = false;
	std::string name = "Achievement";
	std::string description;
};

struct NativeAchievementObjectiveRow
{
	int id = 0;
	int current = 0;
	int required = 0;
	bool completed = false;
	std::string name = "Objective";
};

class NativeAchievementWnd : public CCustomWnd
{
public:
	NativeAchievementWnd() : CCustomWnd((char*)"NativeAchievementWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAchievementWnd);

		SummaryLabel = GetChildItem("NAW_SummaryLabel");
		CategoryList = (CListWnd*)GetChildItem("NAW_CategoryList");
		AchievementList = (CListWnd*)GetChildItem("NAW_AchievementList");
		DetailTitleLabel = GetChildItem("NAW_DetailTitleLabel");
		DetailDescriptionLabel = GetChildItem("NAW_DetailDescriptionLabel");
		ObjectiveList = (CListWnd*)GetChildItem("NAW_ObjectiveList");
		StatusLabel = GetChildItem("NAW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("NAW_RefreshButton");
		CheckButton = (CButtonWnd*)GetChildItem("NAW_CheckButton");

		Layout();
		SetStatus("Open with /achievement, /ach, #achievement, or #ach.");
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
				char command[128];
				sprintf_s(command, "/say #ach native refresh %d %d", SelectedCategoryID, SelectedAchievementID);
				NativeAutoLootSendCommand(command);
				SetStatus("Refreshing achievements...");
				return 1;
			}

			if (pWnd == (CXWnd*)CheckButton) {
				char command[128];
				sprintf_s(command, "/say #ach native check %d %d", SelectedCategoryID, SelectedAchievementID);
				NativeAutoLootSendCommand(command);
				SetStatus("Checking automatic achievements...");
				return 1;
			}

			if (pWnd == (CXWnd*)CategoryList) {
				NativeAchievementCategoryRow* category = GetSelectedCategory();
				if (category && category->id > 0 && category->id != SelectedCategoryID) {
					char command[128];
					sprintf_s(command, "/say #ach native category %d", category->id);
					NativeAutoLootSendCommand(command);
					SetStatus("Loading achievement category...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)AchievementList) {
				NativeAchievementRow* achievement = GetSelectedAchievement();
				if (achievement && achievement->id > 0 && achievement->id != SelectedAchievementID) {
					char command[128];
					sprintf_s(command, "/say #ach native detail %d %d", achievement->id, SelectedCategoryID);
					NativeAutoLootSendCommand(command);
					SetStatus("Loading achievement detail...");
				}
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
		// Achievement resize is handled by SIDL AutoStretch anchors.
	}

	void RefreshRows();
	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text ? text : "");
	}

private:
	NativeAchievementCategoryRow* GetSelectedCategory();
	NativeAchievementRow* GetSelectedAchievement();
	void RefreshCategoryList();
	void RefreshAchievementList();
	void RefreshObjectiveList();
	void SelectCategoryListRow();
	void SelectAchievementListRow();
	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	CXWnd* SummaryLabel = nullptr;
	CListWnd* CategoryList = nullptr;
	CListWnd* AchievementList = nullptr;
	CXWnd* DetailTitleLabel = nullptr;
	CXWnd* DetailDescriptionLabel = nullptr;
	CListWnd* ObjectiveList = nullptr;
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* CheckButton = nullptr;
	int SelectedCategoryID = 0;
	int SelectedAchievementID = 0;
};

static NativeAchievementWnd* gNativeAchievementWnd = nullptr;
static std::vector<NativeAchievementCategoryRow> gNativeAchievementCategories;
static std::vector<NativeAchievementRow> gNativeAchievementRows;
static std::vector<NativeAchievementObjectiveRow> gNativeAchievementObjectives;
static int gNativeAchievementSelectedCategory = 0;
static int gNativeAchievementSelectedAchievement = 0;
static int gNativeAchievementCompleted = 0;
static int gNativeAchievementTotal = 0;
static int gNativeAchievementPoints = 0;
static int gNativeAchievementCategoryCount = 0;
static bool gNativeAchievementLoading = false;
static bool gNativeAchievementCategoriesDirty = true;
static bool gNativeAchievementRowsDirty = true;
static bool gNativeAchievementObjectivesDirty = true;
static std::string gNativeAchievementDetailTitle = "Select an achievement";
static std::string gNativeAchievementDetailDescription = "";

class NativeSpellForgeWnd : public CCustomWnd
{
public:
	NativeSpellForgeWnd() : CCustomWnd((char*)"NativeSpellForgeWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeSpellForgeWnd);

		FormTab = (CButtonWnd*)GetChildItem("ASFW_FormTab");
		DeliveryTab = (CButtonWnd*)GetChildItem("ASFW_DeliveryTab");
		PowerTab = (CButtonWnd*)GetChildItem("ASFW_PowerTab");
		CastingTab = (CButtonWnd*)GetChildItem("ASFW_CastingTab");
		ReviewTab = (CButtonWnd*)GetChildItem("ASFW_ReviewTab");

		FormPage = GetChildItem("ASFW_FormPage");
		DeliveryPage = GetChildItem("ASFW_DeliveryPage");
		PowerPage = GetChildItem("ASFW_PowerPage");
		CastingPage = GetChildItem("ASFW_CastingPage");
		ReviewPage = GetChildItem("ASFW_ReviewPage");

		NameEdit = (CEditWnd*)GetChildItem("ASFW_NameEdit");
		FireButton = (CButtonWnd*)GetChildItem("ASFW_FireButton");
		ColdButton = (CButtonWnd*)GetChildItem("ASFW_ColdButton");
		MagicButton = (CButtonWnd*)GetChildItem("ASFW_MagicButton");
		PoisonButton = (CButtonWnd*)GetChildItem("ASFW_PoisonButton");
		DiseaseButton = (CButtonWnd*)GetChildItem("ASFW_DiseaseButton");
		ElementHintLabel = GetChildItem("ASFW_ElementHintLabel");

		TargetButton = (CButtonWnd*)GetChildItem("ASFW_TargetButton");
		AEButton = (CButtonWnd*)GetChildItem("ASFW_AEButton");
		PBAEButton = (CButtonWnd*)GetChildItem("ASFW_PBAEButton");
		RangeMinusButton = (CButtonWnd*)GetChildItem("ASFW_RangeMinusButton");
		RangePlusButton = (CButtonWnd*)GetChildItem("ASFW_RangePlusButton");
		RangeValueLabel = GetChildItem("ASFW_RangeValueLabel");
		RadiusValueLabel = GetChildItem("ASFW_RadiusValueLabel");
		TargetHintLabel = GetChildItem("ASFW_TargetHintLabel");

		DamageMinusButton = (CButtonWnd*)GetChildItem("ASFW_DamageMinusButton");
		DamagePlusButton = (CButtonWnd*)GetChildItem("ASFW_DamagePlusButton");
		DamageValueLabel = GetChildItem("ASFW_DamageValueLabel");
		ManaValueLabel = GetChildItem("ASFW_ManaValueLabel");
		PowerHintLabel = GetChildItem("ASFW_PowerHintLabel");

		RecastMinusButton = (CButtonWnd*)GetChildItem("ASFW_RecastMinusButton");
		RecastPlusButton = (CButtonWnd*)GetChildItem("ASFW_RecastPlusButton");
		RecastValueLabel = GetChildItem("ASFW_RecastValueLabel");
		CastValueLabel = GetChildItem("ASFW_CastValueLabel");
		CastingHintLabel = GetChildItem("ASFW_CastingHintLabel");

		ReviewNameLabel = GetChildItem("ASFW_ReviewNameLabel");
		ReviewFormLabel = GetChildItem("ASFW_ReviewFormLabel");
		ReviewDeliveryLabel = GetChildItem("ASFW_ReviewDeliveryLabel");
		ReviewPowerLabel = GetChildItem("ASFW_ReviewPowerLabel");
		ReviewCastingLabel = GetChildItem("ASFW_ReviewCastingLabel");
		CreateButton = (CButtonWnd*)GetChildItem("ASFW_CreateButton");
		StatusLabel = GetChildItem("ASFW_StatusLabel");

		SetEditText(DefaultSpellName().c_str());
		ShowPage(PageForm);
		SetStatus("Choose a spell form, then tune delivery and power.");
		UpdateView();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)FormTab) {
				ShowPage(PageForm);
				return 1;
			}

			if (pWnd == (CXWnd*)DeliveryTab) {
				ShowPage(PageDelivery);
				return 1;
			}

			if (pWnd == (CXWnd*)PowerTab) {
				ShowPage(PagePower);
				return 1;
			}

			if (pWnd == (CXWnd*)CastingTab) {
				ShowPage(PageCasting);
				return 1;
			}

			if (pWnd == (CXWnd*)ReviewTab) {
				ShowPage(PageReview);
				return 1;
			}

			if (pWnd == (CXWnd*)FireButton) {
				SelectElement(0);
				return 1;
			}

			if (pWnd == (CXWnd*)ColdButton) {
				SelectElement(1);
				return 1;
			}

			if (pWnd == (CXWnd*)MagicButton) {
				SelectElement(2);
				return 1;
			}

			if (pWnd == (CXWnd*)PoisonButton) {
				SelectElement(3);
				return 1;
			}

			if (pWnd == (CXWnd*)DiseaseButton) {
				SelectElement(4);
				return 1;
			}

			if (pWnd == (CXWnd*)TargetButton) {
				TargetIndex = 0;
				SetStatus("Delivery set to single target.");
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)AEButton) {
				TargetIndex = 1;
				SetStatus("Delivery set to targeted AE.");
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)PBAEButton) {
				TargetIndex = 2;
				SetStatus("Delivery set to point blank AE.");
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)RangeMinusButton) {
				Range = ClampInt(Range - 25, 25, 300);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)RangePlusButton) {
				Range = ClampInt(Range + 25, 25, 300);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)DamageMinusButton) {
				Damage = ClampInt(Damage - 25, 25, 500);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)DamagePlusButton) {
				Damage = ClampInt(Damage + 25, 25, 500);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)RecastMinusButton) {
				Recast = ClampInt(Recast - 1000, 1000, 30000);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)RecastPlusButton) {
				Recast = ClampInt(Recast + 1000, 1000, 30000);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)CreateButton) {
				CreateScroll();
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Open(const std::string& payload)
	{
		(void)payload;
		pXWnd()->Show(1, 1);
		SetStatus("Spell Forge opened. Build a scroll, then scribe it normally.");
		UpdateView();
	}

	void Layout()
	{
	}

private:
	enum ForgePage {
		PageForm = 0,
		PageDelivery = 1,
		PagePower = 2,
		PageCasting = 3,
		PageReview = 4
	};

	static int ClampInt(int value, int minimum, int maximum)
	{
		if (value < minimum) {
			return minimum;
		}

		if (value > maximum) {
			return maximum;
		}

		return value;
	}

	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	void SetButtonText(CButtonWnd* button, const char* text)
	{
		if (button) {
			CXStr value(text ? text : "");
			((CXWnd*)button)->SetWindowTextA(value);
		}
	}

	void SetVisible(CXWnd* wnd, bool visible)
	{
		if (wnd) {
			wnd->Show(visible ? 1 : 0, 1);
		}
	}

	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text);
	}

	const char* ElementKey() const
	{
		switch (ElementIndex) {
		case 1:
			return "cold";
		case 2:
			return "magic";
		case 3:
			return "poison";
		case 4:
			return "disease";
		default:
			return "fire";
		}
	}

	const char* ElementLabel() const
	{
		switch (ElementIndex) {
		case 1:
			return "Cold";
		case 2:
			return "Magic";
		case 3:
			return "Poison";
		case 4:
			return "Disease";
		default:
			return "Fire";
		}
	}

	const char* ElementNoun() const
	{
		switch (ElementIndex) {
		case 1:
			return "Frost";
		case 2:
			return "Arcane";
		case 3:
			return "Venom";
		case 4:
			return "Blight";
		default:
			return "Ember";
		}
	}

	const char* ElementHint() const
	{
		switch (ElementIndex) {
		case 1:
			return "Cold uses a chill resist profile and a blue spell icon.";
		case 2:
			return "Magic is a clean arcane damage pattern.";
		case 3:
			return "Poison uses the poison resist profile for green damage spells.";
		case 4:
			return "Disease uses the disease resist profile and a darker icon.";
		default:
			return "Fire uses the fire resist profile and the ember icon.";
		}
	}

	const char* TargetKey() const
	{
		if (TargetIndex == 1) {
			return "ae";
		}

		if (TargetIndex == 2) {
			return "pbae";
		}

		return "target";
	}

	const char* TargetLabel() const
	{
		if (TargetIndex == 1) {
			return "Targeted AE";
		}

		if (TargetIndex == 2) {
			return "Point Blank AE";
		}

		return "Single Target";
	}

	const char* TargetSuffix() const
	{
		if (TargetIndex == 1) {
			return "Burst";
		}

		if (TargetIndex == 2) {
			return "Bloom";
		}

		return "Lash";
	}

	int Radius() const
	{
		return TargetIndex == 0 ? 0 : 35;
	}

	int ManaCost() const
	{
		return ClampInt(5 + (Damage / 5), 5, 5000);
	}

	int CastTime() const
	{
		return ClampInt(1500 + (Damage / 20), 1500, 6000);
	}

	std::string DefaultSpellName() const
	{
		return std::string(ElementNoun()) + " " + TargetSuffix();
	}

	void SetEditText(const char* text)
	{
		if (!NameEdit) {
			return;
		}

		char buffer[80] = { 0 };
		strcpy_s(buffer, text && text[0] ? text : DefaultSpellName().c_str());
		SetCXStr(&NameEdit->InputText, buffer);
		CXStr value(buffer);
		((CXWnd*)NameEdit)->SetWindowTextA(value);
	}

	std::string ReadName() const
	{
		char text[80] = { 0 };
		if (NameEdit && NameEdit->InputText) {
			GetCXStr(NameEdit->InputText, text, sizeof(text));
		}

		std::string name(text);
		if (name.empty()) {
			name = DefaultSpellName();
		}

		return name;
	}

	std::string EncodeName(const std::string& name) const
	{
		std::string encoded;
		encoded.reserve(name.size());

		for (char ch : name) {
			const bool alpha = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
			const bool digit = ch >= '0' && ch <= '9';
			if (alpha || digit) {
				encoded.push_back(ch);
			}
			else if (ch == ' ' || ch == '_' || ch == '-') {
				if (!encoded.empty() && encoded.back() != '_') {
					encoded.push_back('_');
				}
			}

			if (encoded.size() >= 40) {
				break;
			}
		}

		while (!encoded.empty() && encoded.back() == '_') {
			encoded.pop_back();
		}

		if (encoded.empty()) {
			encoded = DefaultSpellName();
			for (char& ch : encoded) {
				if (ch == ' ') {
					ch = '_';
				}
			}
		}

		return encoded;
	}

	void SelectElement(int index)
	{
		ElementIndex = ClampInt(index, 0, 4);
		SetEditText(DefaultSpellName().c_str());
		SetStatus("Spell form changed.");
		UpdateView();
	}

	void ShowPage(ForgePage page)
	{
		CurrentPage = page;
		SetVisible(FormPage, CurrentPage == PageForm);
		SetVisible(DeliveryPage, CurrentPage == PageDelivery);
		SetVisible(PowerPage, CurrentPage == PagePower);
		SetVisible(CastingPage, CurrentPage == PageCasting);
		SetVisible(ReviewPage, CurrentPage == PageReview);
		UpdateView();
	}

	void UpdateTabs()
	{
		SetButtonText(FormTab, CurrentPage == PageForm ? "> Form <" : "Form");
		SetButtonText(DeliveryTab, CurrentPage == PageDelivery ? "> Delivery <" : "Delivery");
		SetButtonText(PowerTab, CurrentPage == PagePower ? "> Power <" : "Power");
		SetButtonText(CastingTab, CurrentPage == PageCasting ? "> Casting <" : "Casting");
		SetButtonText(ReviewTab, CurrentPage == PageReview ? "> Review <" : "Review");
	}

	void UpdateView()
	{
		UpdateTabs();

		SetButtonText(FireButton, ElementIndex == 0 ? "* Fire" : "Fire");
		SetButtonText(ColdButton, ElementIndex == 1 ? "* Cold" : "Cold");
		SetButtonText(MagicButton, ElementIndex == 2 ? "* Magic" : "Magic");
		SetButtonText(PoisonButton, ElementIndex == 3 ? "* Poison" : "Poison");
		SetButtonText(DiseaseButton, ElementIndex == 4 ? "* Disease" : "Disease");
		SetLabel(ElementHintLabel, ElementHint());

		SetButtonText(TargetButton, TargetIndex == 0 ? "* Single Target" : "Single Target");
		SetButtonText(AEButton, TargetIndex == 1 ? "* Targeted AE" : "Targeted AE");
		SetButtonText(PBAEButton, TargetIndex == 2 ? "* Point Blank AE" : "Point Blank AE");

		char text[192];
		sprintf_s(text, "%d", Range);
		SetLabel(RangeValueLabel, text);
		sprintf_s(text, "%d", Radius());
		SetLabel(RadiusValueLabel, text);
		sprintf_s(text, "%s spells use range %d and radius %d.", TargetLabel(), Range, Radius());
		SetLabel(TargetHintLabel, text);

		sprintf_s(text, "%d", Damage);
		SetLabel(DamageValueLabel, text);
		sprintf_s(text, "%d", ManaCost());
		SetLabel(ManaValueLabel, text);
		sprintf_s(text, "Higher damage raises mana and cast time. Current damage range is 25 to 500.");
		SetLabel(PowerHintLabel, text);

		sprintf_s(text, "%.1f sec", Recast / 1000.0f);
		SetLabel(RecastValueLabel, text);
		sprintf_s(text, "%.1f sec", CastTime() / 1000.0f);
		SetLabel(CastValueLabel, text);
		sprintf_s(text, "Short recasts feel snappy; long recasts are safer for stronger spells.");
		SetLabel(CastingHintLabel, text);

		const std::string name = ReadName();
		SetLabel(ReviewNameLabel, name.c_str());
		sprintf_s(text, "%s damage using %s resist.", ElementLabel(), ElementLabel());
		SetLabel(ReviewFormLabel, text);
		sprintf_s(text, "%s, range %d, radius %d.", TargetLabel(), Range, Radius());
		SetLabel(ReviewDeliveryLabel, text);
		sprintf_s(text, "%d damage for %d mana.", Damage, ManaCost());
		SetLabel(ReviewPowerLabel, text);
		sprintf_s(text, "%.1f sec cast, %.1f sec recast.", CastTime() / 1000.0f, Recast / 1000.0f);
		SetLabel(ReviewCastingLabel, text);
	}

	void CreateScroll()
	{
		const std::string name = ReadName();
		const std::string encoded_name = EncodeName(name);

		char command[256];
		sprintf_s(
			command,
			"/say #livespell craft element=%s target=%s range=%d damage=%d recast=%d name=%s",
			ElementKey(),
			TargetKey(),
			Range,
			Damage,
			Recast,
			encoded_name.c_str()
		);

		NativeAutoLootSendCommand(command);
		SetStatus("Creating scroll. Watch chat for the new item.");
	}

	ForgePage CurrentPage = PageForm;
	int ElementIndex = 1;
	int TargetIndex = 0;
	int Range = 200;
	int Damage = 100;
	int Recast = 3000;

	CButtonWnd* FormTab = nullptr;
	CButtonWnd* DeliveryTab = nullptr;
	CButtonWnd* PowerTab = nullptr;
	CButtonWnd* CastingTab = nullptr;
	CButtonWnd* ReviewTab = nullptr;
	CXWnd* FormPage = nullptr;
	CXWnd* DeliveryPage = nullptr;
	CXWnd* PowerPage = nullptr;
	CXWnd* CastingPage = nullptr;
	CXWnd* ReviewPage = nullptr;
	CEditWnd* NameEdit = nullptr;
	CButtonWnd* FireButton = nullptr;
	CButtonWnd* ColdButton = nullptr;
	CButtonWnd* MagicButton = nullptr;
	CButtonWnd* PoisonButton = nullptr;
	CButtonWnd* DiseaseButton = nullptr;
	CXWnd* ElementHintLabel = nullptr;
	CButtonWnd* TargetButton = nullptr;
	CButtonWnd* AEButton = nullptr;
	CButtonWnd* PBAEButton = nullptr;
	CButtonWnd* RangeMinusButton = nullptr;
	CButtonWnd* RangePlusButton = nullptr;
	CXWnd* RangeValueLabel = nullptr;
	CXWnd* RadiusValueLabel = nullptr;
	CXWnd* TargetHintLabel = nullptr;
	CButtonWnd* DamageMinusButton = nullptr;
	CButtonWnd* DamagePlusButton = nullptr;
	CXWnd* DamageValueLabel = nullptr;
	CXWnd* ManaValueLabel = nullptr;
	CXWnd* PowerHintLabel = nullptr;
	CButtonWnd* RecastMinusButton = nullptr;
	CButtonWnd* RecastPlusButton = nullptr;
	CXWnd* RecastValueLabel = nullptr;
	CXWnd* CastValueLabel = nullptr;
	CXWnd* CastingHintLabel = nullptr;
	CXWnd* ReviewNameLabel = nullptr;
	CXWnd* ReviewFormLabel = nullptr;
	CXWnd* ReviewDeliveryLabel = nullptr;
	CXWnd* ReviewPowerLabel = nullptr;
	CXWnd* ReviewCastingLabel = nullptr;
	CButtonWnd* CreateButton = nullptr;
	CXWnd* StatusLabel = nullptr;
};

static NativeSpellForgeWnd* gNativeSpellForgeWnd = nullptr;

static void NativeSpellForgeShowWindow(const std::string& payload)
{
	if (!gNativeSpellForgeWnd) {
		NativeAutoLootTrace("creating spell forge window");
		gNativeSpellForgeWnd = new NativeSpellForgeWnd();
	}

	gNativeSpellForgeWnd->Open(payload);
}

class NativeItemForgeWnd : public CCustomWnd
{
public:
	NativeItemForgeWnd() : CCustomWnd((char*)"NativeItemForgeWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeItemForgeWnd);

		FormTab = (CButtonWnd*)GetChildItem("AIFW_FormTab");
		StatsTab = (CButtonWnd*)GetChildItem("AIFW_StatsTab");
		CombatTab = (CButtonWnd*)GetChildItem("AIFW_CombatTab");
		ReviewTab = (CButtonWnd*)GetChildItem("AIFW_ReviewTab");
		FormPage = GetChildItem("AIFW_FormPage");
		StatsPage = GetChildItem("AIFW_StatsPage");
		CombatPage = GetChildItem("AIFW_CombatPage");
		ReviewPage = GetChildItem("AIFW_ReviewPage");

		NameEdit = (CEditWnd*)GetChildItem("AIFW_NameEdit");
		WeaponButton = (CButtonWnd*)GetChildItem("AIFW_WeaponButton");
		ArmorButton = (CButtonWnd*)GetChildItem("AIFW_ArmorButton");
		JewelryButton = (CButtonWnd*)GetChildItem("AIFW_JewelryButton");
		CharmButton = (CButtonWnd*)GetChildItem("AIFW_CharmButton");
		ShieldButton = (CButtonWnd*)GetChildItem("AIFW_ShieldButton");
		TypeHintLabel = GetChildItem("AIFW_TypeHintLabel");

		HPMinusButton = (CButtonWnd*)GetChildItem("AIFW_HPMinusButton");
		HPPlusButton = (CButtonWnd*)GetChildItem("AIFW_HPPlusButton");
		HPValueLabel = GetChildItem("AIFW_HPValueLabel");
		ManaMinusButton = (CButtonWnd*)GetChildItem("AIFW_ManaMinusButton");
		ManaPlusButton = (CButtonWnd*)GetChildItem("AIFW_ManaPlusButton");
		ManaValueLabel = GetChildItem("AIFW_ManaValueLabel");
		ACMinusButton = (CButtonWnd*)GetChildItem("AIFW_ACMinusButton");
		ACPlusButton = (CButtonWnd*)GetChildItem("AIFW_ACPlusButton");
		ACValueLabel = GetChildItem("AIFW_ACValueLabel");
		StatsHintLabel = GetChildItem("AIFW_StatsHintLabel");

		DamageMinusButton = (CButtonWnd*)GetChildItem("AIFW_DamageMinusButton");
		DamagePlusButton = (CButtonWnd*)GetChildItem("AIFW_DamagePlusButton");
		DamageValueLabel = GetChildItem("AIFW_DamageValueLabel");
		DelayMinusButton = (CButtonWnd*)GetChildItem("AIFW_DelayMinusButton");
		DelayPlusButton = (CButtonWnd*)GetChildItem("AIFW_DelayPlusButton");
		DelayValueLabel = GetChildItem("AIFW_DelayValueLabel");
		HasteMinusButton = (CButtonWnd*)GetChildItem("AIFW_HasteMinusButton");
		HastePlusButton = (CButtonWnd*)GetChildItem("AIFW_HastePlusButton");
		HasteValueLabel = GetChildItem("AIFW_HasteValueLabel");
		CombatHintLabel = GetChildItem("AIFW_CombatHintLabel");

		ReviewNameLabel = GetChildItem("AIFW_ReviewNameLabel");
		ReviewTypeLabel = GetChildItem("AIFW_ReviewTypeLabel");
		ReviewStatsLabel = GetChildItem("AIFW_ReviewStatsLabel");
		ReviewCombatLabel = GetChildItem("AIFW_ReviewCombatLabel");
		CreateButton = (CButtonWnd*)GetChildItem("AIFW_CreateButton");
		StatusLabel = GetChildItem("AIFW_StatusLabel");

		SetEditText(DefaultItemName().c_str());
		ShowPage(PageForm);
		SetStatus("Choose an item form, then tune stats and combat values.");
		UpdateView();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)FormTab) {
				ShowPage(PageForm);
				return 1;
			}

			if (pWnd == (CXWnd*)StatsTab) {
				ShowPage(PageStats);
				return 1;
			}

			if (pWnd == (CXWnd*)CombatTab) {
				ShowPage(PageCombat);
				return 1;
			}

			if (pWnd == (CXWnd*)ReviewTab) {
				ShowPage(PageReview);
				return 1;
			}

			if (pWnd == (CXWnd*)WeaponButton) {
				SelectType(0);
				return 1;
			}

			if (pWnd == (CXWnd*)ArmorButton) {
				SelectType(1);
				return 1;
			}

			if (pWnd == (CXWnd*)JewelryButton) {
				SelectType(2);
				return 1;
			}

			if (pWnd == (CXWnd*)CharmButton) {
				SelectType(3);
				return 1;
			}

			if (pWnd == (CXWnd*)ShieldButton) {
				SelectType(4);
				return 1;
			}

			if (pWnd == (CXWnd*)HPMinusButton) {
				HP = ClampInt(HP - 25, 0, 500);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)HPPlusButton) {
				HP = ClampInt(HP + 25, 0, 500);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)ManaMinusButton) {
				Mana = ClampInt(Mana - 25, 0, 500);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)ManaPlusButton) {
				Mana = ClampInt(Mana + 25, 0, 500);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)ACMinusButton) {
				AC = ClampInt(AC - 5, 0, 100);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)ACPlusButton) {
				AC = ClampInt(AC + 5, 0, 100);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)DamageMinusButton) {
				Damage = ClampInt(Damage - 2, 0, 100);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)DamagePlusButton) {
				Damage = ClampInt(Damage + 2, 0, 100);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)DelayMinusButton) {
				Delay = ClampInt(Delay - 2, 10, 60);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)DelayPlusButton) {
				Delay = ClampInt(Delay + 2, 10, 60);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)HasteMinusButton) {
				Haste = ClampInt(Haste - 5, 0, 50);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)HastePlusButton) {
				Haste = ClampInt(Haste + 5, 0, 50);
				UpdateView();
				return 1;
			}

			if (pWnd == (CXWnd*)CreateButton) {
				CreateItem();
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Open(const std::string& payload)
	{
		(void)payload;
		pXWnd()->Show(1, 1);
		SetStatus("Item Forge opened. Create will summon the finished item.");
		UpdateView();
	}

	void Layout()
	{
	}

private:
	enum ItemPage {
		PageForm = 0,
		PageStats = 1,
		PageCombat = 2,
		PageReview = 3
	};

	static int ClampInt(int value, int minimum, int maximum)
	{
		if (value < minimum) {
			return minimum;
		}

		if (value > maximum) {
			return maximum;
		}

		return value;
	}

	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	void SetButtonText(CButtonWnd* button, const char* text)
	{
		if (button) {
			CXStr value(text ? text : "");
			((CXWnd*)button)->SetWindowTextA(value);
		}
	}

	void SetVisible(CXWnd* wnd, bool visible)
	{
		if (wnd) {
			wnd->Show(visible ? 1 : 0, 1);
		}
	}

	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text);
	}

	const char* TypeKey() const
	{
		switch (TypeIndex) {
		case 1:
			return "armor";
		case 2:
			return "jewelry";
		case 3:
			return "charm";
		case 4:
			return "shield";
		default:
			return "weapon";
		}
	}

	const char* TypeLabel() const
	{
		switch (TypeIndex) {
		case 1:
			return "Armor";
		case 2:
			return "Jewelry";
		case 3:
			return "Charm";
		case 4:
			return "Shield";
		default:
			return "Weapon";
		}
	}

	const char* TypeSuffix() const
	{
		switch (TypeIndex) {
		case 1:
			return "Cuirass";
		case 2:
			return "Ring";
		case 3:
			return "Charm";
		case 4:
			return "Shield";
		default:
			return "Blade";
		}
	}

	const char* TypeHint() const
	{
		switch (TypeIndex) {
		case 1:
			return "Armor becomes a chest piece and favors AC over weapon values.";
		case 2:
			return "Jewelry becomes a ring and carries clean HP, mana, and AC stats.";
		case 3:
			return "Charms equip in the charm slot and ignore combat weapon values.";
		case 4:
			return "Shields equip in secondary and turn AC into the main stat.";
		default:
			return "Weapons equip primary or secondary and use damage, delay, and haste.";
		}
	}

	bool UsesCombatValues() const
	{
		return TypeIndex == 0;
	}

	std::string DefaultItemName() const
	{
		return std::string("Forged ") + TypeSuffix();
	}

	void SetEditText(const char* text)
	{
		if (!NameEdit) {
			return;
		}

		char buffer[80] = { 0 };
		strcpy_s(buffer, text && text[0] ? text : DefaultItemName().c_str());
		SetCXStr(&NameEdit->InputText, buffer);
		CXStr value(buffer);
		((CXWnd*)NameEdit)->SetWindowTextA(value);
	}

	std::string ReadName() const
	{
		char text[80] = { 0 };
		if (NameEdit && NameEdit->InputText) {
			GetCXStr(NameEdit->InputText, text, sizeof(text));
		}

		std::string name(text);
		if (name.empty()) {
			name = DefaultItemName();
		}

		return name;
	}

	std::string EncodeName(const std::string& name) const
	{
		std::string encoded;
		encoded.reserve(name.size());

		for (char ch : name) {
			const bool alpha = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
			const bool digit = ch >= '0' && ch <= '9';
			if (alpha || digit) {
				encoded.push_back(ch);
			}
			else if (ch == ' ' || ch == '_' || ch == '-') {
				if (!encoded.empty() && encoded.back() != '_') {
					encoded.push_back('_');
				}
			}

			if (encoded.size() >= 40) {
				break;
			}
		}

		while (!encoded.empty() && encoded.back() == '_') {
			encoded.pop_back();
		}

		if (encoded.empty()) {
			encoded = DefaultItemName();
			for (char& ch : encoded) {
				if (ch == ' ') {
					ch = '_';
				}
			}
		}

		return encoded;
	}

	void SelectType(int index)
	{
		TypeIndex = ClampInt(index, 0, 4);
		SetEditText(DefaultItemName().c_str());
		SetStatus("Item form changed.");
		UpdateView();
	}

	void ShowPage(ItemPage page)
	{
		CurrentPage = page;
		SetVisible(FormPage, CurrentPage == PageForm);
		SetVisible(StatsPage, CurrentPage == PageStats);
		SetVisible(CombatPage, CurrentPage == PageCombat);
		SetVisible(ReviewPage, CurrentPage == PageReview);
		UpdateView();
	}

	void UpdateTabs()
	{
		SetButtonText(FormTab, CurrentPage == PageForm ? "> Form <" : "Form");
		SetButtonText(StatsTab, CurrentPage == PageStats ? "> Stats <" : "Stats");
		SetButtonText(CombatTab, CurrentPage == PageCombat ? "> Combat <" : "Combat");
		SetButtonText(ReviewTab, CurrentPage == PageReview ? "> Review <" : "Review");
	}

	void UpdateView()
	{
		UpdateTabs();
		SetButtonText(WeaponButton, TypeIndex == 0 ? "* Weapon" : "Weapon");
		SetButtonText(ArmorButton, TypeIndex == 1 ? "* Armor" : "Armor");
		SetButtonText(JewelryButton, TypeIndex == 2 ? "* Jewelry" : "Jewelry");
		SetButtonText(CharmButton, TypeIndex == 3 ? "* Charm" : "Charm");
		SetButtonText(ShieldButton, TypeIndex == 4 ? "* Shield" : "Shield");
		SetLabel(TypeHintLabel, TypeHint());

		char text[192];
		sprintf_s(text, "%d", HP);
		SetLabel(HPValueLabel, text);
		sprintf_s(text, "%d", Mana);
		SetLabel(ManaValueLabel, text);
		sprintf_s(text, "%d", AC);
		SetLabel(ACValueLabel, text);
		sprintf_s(text, "Stat ranges: HP 0-500, Mana 0-500, AC 0-100.");
		SetLabel(StatsHintLabel, text);

		sprintf_s(text, "%d", UsesCombatValues() ? Damage : 0);
		SetLabel(DamageValueLabel, text);
		sprintf_s(text, "%d", UsesCombatValues() ? Delay : 0);
		SetLabel(DelayValueLabel, text);
		sprintf_s(text, "%d%%", UsesCombatValues() ? Haste : 0);
		SetLabel(HasteValueLabel, text);
		SetLabel(CombatHintLabel, UsesCombatValues() ? "Weapon values apply to blades. Other forms keep these at zero." : "This item form ignores weapon damage, delay, and haste.");

		const std::string name = ReadName();
		SetLabel(ReviewNameLabel, name.c_str());
		sprintf_s(text, "%s item, summoned directly to your cursor.", TypeLabel());
		SetLabel(ReviewTypeLabel, text);
		sprintf_s(text, "HP %d, Mana %d, AC %d.", HP, Mana, AC);
		SetLabel(ReviewStatsLabel, text);
		sprintf_s(text, "Damage %d, Delay %d, Haste %d%%.", UsesCombatValues() ? Damage : 0, UsesCombatValues() ? Delay : 0, UsesCombatValues() ? Haste : 0);
		SetLabel(ReviewCombatLabel, text);
	}

	void CreateItem()
	{
		const std::string encoded_name = EncodeName(ReadName());
		char command[256];
		sprintf_s(
			command,
			"/say #itemforge craft type=%s hp=%d mana=%d ac=%d damage=%d delay=%d haste=%d name=%s",
			TypeKey(),
			HP,
			Mana,
			AC,
			UsesCombatValues() ? Damage : 0,
			UsesCombatValues() ? Delay : 24,
			UsesCombatValues() ? Haste : 0,
			encoded_name.c_str()
		);

		NativeAutoLootSendCommand(command);
		SetStatus("Creating item. Watch chat and your cursor.");
	}

	ItemPage CurrentPage = PageForm;
	int TypeIndex = 0;
	int HP = 50;
	int Mana = 25;
	int AC = 5;
	int Damage = 8;
	int Delay = 24;
	int Haste = 0;

	CButtonWnd* FormTab = nullptr;
	CButtonWnd* StatsTab = nullptr;
	CButtonWnd* CombatTab = nullptr;
	CButtonWnd* ReviewTab = nullptr;
	CXWnd* FormPage = nullptr;
	CXWnd* StatsPage = nullptr;
	CXWnd* CombatPage = nullptr;
	CXWnd* ReviewPage = nullptr;
	CEditWnd* NameEdit = nullptr;
	CButtonWnd* WeaponButton = nullptr;
	CButtonWnd* ArmorButton = nullptr;
	CButtonWnd* JewelryButton = nullptr;
	CButtonWnd* CharmButton = nullptr;
	CButtonWnd* ShieldButton = nullptr;
	CXWnd* TypeHintLabel = nullptr;
	CButtonWnd* HPMinusButton = nullptr;
	CButtonWnd* HPPlusButton = nullptr;
	CXWnd* HPValueLabel = nullptr;
	CButtonWnd* ManaMinusButton = nullptr;
	CButtonWnd* ManaPlusButton = nullptr;
	CXWnd* ManaValueLabel = nullptr;
	CButtonWnd* ACMinusButton = nullptr;
	CButtonWnd* ACPlusButton = nullptr;
	CXWnd* ACValueLabel = nullptr;
	CXWnd* StatsHintLabel = nullptr;
	CButtonWnd* DamageMinusButton = nullptr;
	CButtonWnd* DamagePlusButton = nullptr;
	CXWnd* DamageValueLabel = nullptr;
	CButtonWnd* DelayMinusButton = nullptr;
	CButtonWnd* DelayPlusButton = nullptr;
	CXWnd* DelayValueLabel = nullptr;
	CButtonWnd* HasteMinusButton = nullptr;
	CButtonWnd* HastePlusButton = nullptr;
	CXWnd* HasteValueLabel = nullptr;
	CXWnd* CombatHintLabel = nullptr;
	CXWnd* ReviewNameLabel = nullptr;
	CXWnd* ReviewTypeLabel = nullptr;
	CXWnd* ReviewStatsLabel = nullptr;
	CXWnd* ReviewCombatLabel = nullptr;
	CButtonWnd* CreateButton = nullptr;
	CXWnd* StatusLabel = nullptr;
};

static NativeItemForgeWnd* gNativeItemForgeWnd = nullptr;

static void NativeItemForgeShowWindow(const std::string& payload)
{
	if (!gNativeItemForgeWnd) {
		NativeAutoLootTrace("creating item forge window");
		gNativeItemForgeWnd = new NativeItemForgeWnd();
	}

	gNativeItemForgeWnd->Open(payload);
}

void NativeAchievementWnd::RefreshCategoryList()
{
	if (!CategoryList) {
		return;
	}

	CategoryList->DeleteAll();
	if (gNativeAchievementCategories.empty()) {
		CXStr dash("-");
		const int row = CategoryList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No categories loaded.");
		CategoryList->SetItemText(row, 1, &empty);
		CategoryList->SetItemText(row, 2, &dash);
		gNativeAchievementCategoriesDirty = false;
		return;
	}

	int selected_row = -1;
	for (const NativeAchievementCategoryRow& category : gNativeAchievementCategories) {
		std::string display = category.parent_id ? "  " + category.name : category.name;
		CXStr name(display.c_str());
		const COLORREF row_color = category.total <= 0 ? 0xFFB0B0B0 :
			(category.completed >= category.total ? 0xFF66FF66 : 0xFFFFFFFF);
		const int row = CategoryList->AddString(name, row_color, (uint32_t)category.id, nullptr, nullptr);

		char progress_text[32];
		sprintf_s(progress_text, "%d/%d", category.completed, category.total);
		char points_text[32];
		sprintf_s(points_text, "%d", category.points);
		CXStr progress(progress_text);
		CXStr points(points_text);
		CategoryList->SetItemText(row, 1, &progress);
		CategoryList->SetItemText(row, 2, &points);

		if (category.id == SelectedCategoryID) {
			selected_row = row;
		}
	}

	if (selected_row >= 0) {
		CategoryList->SetCurSel(selected_row);
	}

	gNativeAchievementCategoriesDirty = false;
}

void NativeAchievementWnd::RefreshAchievementList()
{
	if (!AchievementList) {
		return;
	}

	AchievementList->DeleteAll();
	if (gNativeAchievementRows.empty()) {
		CXStr dash("-");
		const int row = AchievementList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No achievements in this category.");
		AchievementList->SetItemText(row, 1, &empty);
		AchievementList->SetItemText(row, 2, &dash);
		gNativeAchievementRowsDirty = false;
		return;
	}

	int selected_row = -1;
	for (const NativeAchievementRow& achievement : gNativeAchievementRows) {
		CXStr state(achievement.completed ? "Done" : "Open");
		const COLORREF row_color = achievement.completed ? 0xFF66FF66 : 0xFFFFFFFF;
		const int row = AchievementList->AddString(state, row_color, (uint32_t)achievement.id, nullptr, nullptr);

		char points_text[32];
		sprintf_s(points_text, "%d", achievement.points);
		CXStr name(achievement.name.c_str());
		CXStr points(points_text);
		CXStr description(achievement.description.c_str());
		AchievementList->SetItemText(row, 1, &name);
		AchievementList->SetItemText(row, 2, &points);
		AchievementList->SetItemText(row, 3, &description);

		if (achievement.id == SelectedAchievementID) {
			selected_row = row;
		}
	}

	if (selected_row >= 0) {
		AchievementList->SetCurSel(selected_row);
	}

	gNativeAchievementRowsDirty = false;
}

void NativeAchievementWnd::RefreshObjectiveList()
{
	if (!ObjectiveList) {
		return;
	}

	ObjectiveList->DeleteAll();
	if (gNativeAchievementObjectives.empty()) {
		CXStr dash("-");
		const int row = ObjectiveList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No objectives loaded.");
		ObjectiveList->SetItemText(row, 1, &empty);
		ObjectiveList->SetItemText(row, 2, &dash);
		gNativeAchievementObjectivesDirty = false;
		return;
	}

	for (const NativeAchievementObjectiveRow& objective : gNativeAchievementObjectives) {
		CXStr state(objective.completed ? "Done" : "Open");
		const int row = ObjectiveList->AddString(state, objective.completed ? 0xFF66FF66 : 0xFFFFFFFF, (uint32_t)objective.id, nullptr, nullptr);

		char progress_text[32];
		sprintf_s(progress_text, "%d/%d", objective.current, objective.required);
		CXStr name(objective.name.c_str());
		CXStr progress(progress_text);
		ObjectiveList->SetItemText(row, 1, &name);
		ObjectiveList->SetItemText(row, 2, &progress);
	}

	gNativeAchievementObjectivesDirty = false;
}

void NativeAchievementWnd::SelectCategoryListRow()
{
	if (!CategoryList || SelectedCategoryID <= 0) {
		return;
	}

	for (size_t i = 0; i < gNativeAchievementCategories.size(); ++i) {
		if (gNativeAchievementCategories[i].id == SelectedCategoryID) {
			CategoryList->SetCurSel((int)i);
			return;
		}
	}
}

void NativeAchievementWnd::SelectAchievementListRow()
{
	if (!AchievementList || SelectedAchievementID <= 0) {
		return;
	}

	for (size_t i = 0; i < gNativeAchievementRows.size(); ++i) {
		if (gNativeAchievementRows[i].id == SelectedAchievementID) {
			AchievementList->SetCurSel((int)i);
			return;
		}
	}
}

void NativeAchievementWnd::RefreshRows()
{
	SelectedCategoryID = gNativeAchievementSelectedCategory;
	SelectedAchievementID = gNativeAchievementSelectedAchievement;

	char summary[160];
	sprintf_s(
		summary,
		"Achievements %d/%d complete  Points %d  Categories %d",
		gNativeAchievementCompleted,
		gNativeAchievementTotal,
		gNativeAchievementPoints,
		gNativeAchievementCategoryCount
	);
	SetLabel(SummaryLabel, summary);
	SetLabel(DetailTitleLabel, gNativeAchievementDetailTitle.c_str());
	SetLabel(DetailDescriptionLabel, gNativeAchievementDetailDescription.c_str());

	if (gNativeAchievementCategoriesDirty) {
		RefreshCategoryList();
	}
	else {
		SelectCategoryListRow();
	}

	if (gNativeAchievementRowsDirty) {
		RefreshAchievementList();
	}
	else {
		SelectAchievementListRow();
	}

	if (gNativeAchievementObjectivesDirty) {
		RefreshObjectiveList();
	}
}

NativeAchievementCategoryRow* NativeAchievementWnd::GetSelectedCategory()
{
	if (!CategoryList) {
		return nullptr;
	}

	const int selected = CategoryList->GetCurSel();
	if (selected < 0) {
		return nullptr;
	}

	const int category_id = (int)CategoryList->GetItemData(selected);
	if (category_id <= 0) {
		return nullptr;
	}

	for (NativeAchievementCategoryRow& category : gNativeAchievementCategories) {
		if (category.id == category_id) {
			return &category;
		}
	}

	return nullptr;
}

NativeAchievementRow* NativeAchievementWnd::GetSelectedAchievement()
{
	if (!AchievementList) {
		return nullptr;
	}

	const int selected = AchievementList->GetCurSel();
	if (selected < 0) {
		return nullptr;
	}

	const int achievement_id = (int)AchievementList->GetItemData(selected);
	if (achievement_id <= 0) {
		return nullptr;
	}

	for (NativeAchievementRow& achievement : gNativeAchievementRows) {
		if (achievement.id == achievement_id) {
			return &achievement;
		}
	}

	return nullptr;
}

static void NativeAchievementEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeAchievementWnd) {
		NativeAutoLootTrace("creating achievement window");
		gNativeAchievementWnd = new NativeAchievementWnd();
		gNativeAchievementWnd->RefreshRows();
		NativeAutoLootTrace("achievement window created");
	}

	if (show && gNativeAchievementWnd) {
		gNativeAchievementWnd->pXWnd()->Show(1, 1);
	}
}

void NativeAutoLootRulesWnd::SetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text);
		label->SetWindowTextA(value);
	}
}

void NativeAutoLootRulesWnd::Layout()
{
	// Filters resize is handled by SIDL AutoStretch anchors. Avoid manually
	// moving children during construction; this client build can fault there.
}

void NativeAutoLootRulesWnd::RefreshRows()
{
	if (!RuleList) {
		return;
	}

	RuleList->DeleteAll();

	char summary[96];
	sprintf_s(summary, "Rules %d keep / %d never", gNativeAutoLootKeepCount, gNativeAutoLootIgnoreCount);
	SetLabel(SummaryLabel, summary);

	if (gNativeAutoLootRuleRows.empty()) {
		CXStr rule("-");
		const int row = RuleList->AddString(rule, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr item("No filters loaded.");
		CXStr item_id("-");
		RuleList->SetItemText(row, 1, &item);
		RuleList->SetItemText(row, 2, &item_id);
		return;
	}

	for (const NativeAutoLootRuleRow& entry : gNativeAutoLootRuleRows) {
		CXStr rule(NativeDisplayRule(entry.rule));
		const int row = RuleList->AddString(rule, NativeIsNeverRule(entry.rule) ? 0xFFFF8080 : 0xFF66FF66, (uint32_t)entry.item_id, nullptr, nullptr);

		char item_id[32];
		sprintf_s(item_id, "%d", entry.item_id);
		CXStr item(entry.item.c_str());
		CXStr id(item_id);
		RuleList->SetItemText(row, 1, &item);
		RuleList->SetItemText(row, 2, &id);
	}
}

NativeAutoLootRuleRow* NativeAutoLootRulesWnd::GetSelectedRule()
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

	for (NativeAutoLootRuleRow& row : gNativeAutoLootRuleRows) {
		if (row.item_id == item_id) {
			return &row;
		}
	}

	return nullptr;
}

static void NativeAutoLootShowRulesWindow()
{
	if (!gNativeAutoLootRulesWnd) {
		NativeAutoLootTrace("creating rules window");
		gNativeAutoLootRulesWnd = new NativeAutoLootRulesWnd();
	}

	gNativeAutoLootRulesWnd->RefreshRows();
	gNativeAutoLootRulesWnd->pXWnd()->Show(1, 1);
	gNativeAutoLootRulesWnd->SetStatus("Refreshing filters...");
	NativeAutoLootSendCommand("/say #lootfilter native list both");
}

static void NativeAutoLootEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeAutoLootWnd) {
		NativeAutoLootTrace("creating window");
		gNativeAutoLootWnd = new NativeAutoLootWnd();
		gNativeAutoLootWnd->RefreshRows();
		NativeAutoLootTrace("window created");
	}

	if (show && gNativeAutoLootWnd) {
		gNativeAutoLootWnd->pXWnd()->Show(1, 1);
	}
}

void NativeAutoLootWnd::SetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text);
		label->SetWindowTextA(value);
	}
}

void NativeAutoLootWnd::Layout()
{
	// Main-window resize is handled by SIDL AutoStretch anchors. Moving many
	// child controls manually during construction can fault this client build.
}

void NativeAutoLootWnd::RefreshList(CListWnd* list, bool shared)
{
	if (!list) {
		return;
	}

	list->DeleteAll();
	int visible = 0;

	for (const NativeAutoLootRow& entry : gNativeAutoLootRows) {
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
			CXStr rule(NativeDisplayRule(entry.rule));
			CXStr status(entry.locked ? "Locked" : entry.state.c_str());
			list->SetItemText(row, 1, &qty);
			list->SetItemText(row, 2, &source);
			list->SetItemText(row, 3, &rule);
			list->SetItemText(row, 4, &status);
			list->SetItemColor(row, 3, NativeIsNeverRule(entry.rule) ? 0xFFFF8080 : 0xFF66FF66);
			list->SetItemColor(row, 4, entry.locked ? 0xFFFF8080 : 0xFFFFFFFF);
		}
		else {
			char qty_text[16];
			sprintf_s(qty_text, "%d", entry.qty);
			CXStr qty(qty_text);
			CXStr source(entry.source.c_str());
			CXStr status(entry.locked ? "Locked" : entry.state.c_str());
			CXStr rule(NativeDisplayRule(entry.rule));
			CXStr nd(entry.state == "need" || entry.state == "alwaysneed" ? "X" : "");
			CXStr gd(entry.state == "greed" || entry.state == "alwaysgreed" ? "X" : "");
			CXStr no(entry.state == "no" ? "X" : "");
			CXStr an(entry.state == "alwaysneed" ? "X" : NativeShortRule(entry.rule));
			CXStr ag(entry.state == "alwaysgreed" ? "X" : "");
			CXStr nv(NativeIsNeverRule(entry.rule) ? "X" : "");
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
			list->SetItemColor(row, 4, NativeIsNeverRule(entry.rule) ? 0xFFFF8080 : 0xFF66FF66);
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
		CXStr empty(gNativeAutoLootGrouped ? "No shared loot." : "Not grouped.");
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

void NativeAutoLootWnd::RefreshRows()
{
	RefreshList(PersonalList, false);
	RefreshList(SharedList, true);

	char rules[96];
	sprintf_s(rules, "Rules %d keep / %d never", gNativeAutoLootKeepCount, gNativeAutoLootIgnoreCount);
	SetLabel(RuleSummaryLabel, rules);

	char master[128];
	sprintf_s(master, "Group: %s  Assigned: %s", gNativeAutoLootGroupMode.c_str(), gNativeAutoLootAssigned.c_str());
	SetLabel(MasterLabel, master);

	if (ApplyFiltersCheck) {
		CXStr value(gNativeAutoLootEnabled ? "AutoLoot On" : "AutoLoot Off");
		((CXWnd*)ApplyFiltersCheck)->SetWindowTextA(value);
	}
}

NativeAutoLootRow* NativeAutoLootWnd::GetSelectedRow()
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

	for (NativeAutoLootRow& row : gNativeAutoLootRows) {
		if (row.entry_id == entry_id) {
			return &row;
		}
	}

	return nullptr;
}

static void NativeAutoLootSendCommand(const char* command)
{
	if (!command || !command[0] || !pEverQuest || !pLocalPlayer) {
		return;
	}

	NativeAutoLootTrace("send command: %s", command);
	char buffer[256];
	strcpy_s(buffer, command);
	pEverQuest->InterpretCmd((EQPlayer*)pLocalPlayer, buffer);
}

static bool NativeCommandMatch(const char* line, const char* command, const char** arguments)
{
	if (!line || !command) {
		return false;
	}

	const size_t command_length = strlen(command);
	if (strnicmp(line, command, command_length) != 0) {
		return false;
	}

	const char next = line[command_length];
	if (next != 0 && next != ' ' && next != '\t') {
		return false;
	}

	if (arguments) {
		const char* current = line + command_length;
		while (*current == ' ' || *current == '\t') {
			++current;
		}
		*arguments = current;
	}

	return true;
}

static bool NativeAchievementRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/achievement", &arguments) &&
		!NativeCommandMatch(line, "/achievements", &arguments) &&
		!NativeCommandMatch(line, "/achivement", &arguments) &&
		!NativeCommandMatch(line, "/achivements", &arguments) &&
		!NativeCommandMatch(line, "/ach", &arguments)
	) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		strcpy_s(output, output_size, "/say #ach native");
		return true;
	}

	if (
		NativeCommandMatch(arguments, "open", nullptr) ||
		NativeCommandMatch(arguments, "window", nullptr) ||
		NativeCommandMatch(arguments, "ui", nullptr) ||
		NativeCommandMatch(arguments, "panel", nullptr)
	) {
		strcpy_s(output, output_size, "/say #ach native");
		return true;
	}

	sprintf_s(output, output_size, "/say #ach %s", arguments);
	return true;
}

class NativeAutoLootCommandHook
{
public:
	VOID Trampoline(EQPlayer* player, PCHAR line);
	VOID Detour(EQPlayer* player, PCHAR line)
	{
		char rewritten[256];
		if (NativeAchievementRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		Trampoline(player, line);
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID NativeAutoLootCommandHook::Trampoline(EQPlayer* player, PCHAR line));

static void NativeAutoLootUpdateWindow()
{
	if (!gNativeAutoLootWnd) {
		return;
	}

	int personal = 0;
	int shared = 0;
	for (const NativeAutoLootRow& row : gNativeAutoLootRows) {
		if (row.shared) {
			++shared;
		}
		else {
			++personal;
		}
	}

	gNativeAutoLootWnd->RefreshRows();
	char status[128];
	sprintf_s(status, "Personal %d / Shared %d", personal, shared);
	gNativeAutoLootWnd->SetStatus(status);
}

static bool NativeApplyLiveSpellPatch(const std::string& payload)
{
	const int spell_id = NativeToInt(NativeGetPairValue(payload, "id"));
	const int base_spell_id = NativeToInt(NativeGetPairValue(payload, "base"));
	const int version = NativeToInt(NativeGetPairValue(payload, "version"));

	if (!pSpellMgr || spell_id <= 0 || base_spell_id <= 0 || spell_id >= TOTAL_SPELL_COUNT || base_spell_id >= TOTAL_SPELL_COUNT) {
		NativeAutoLootTrace("livespell rejected invalid ids: id=%d base=%d", spell_id, base_spell_id);
		return true;
	}

	PSPELLMGR spell_mgr = (PSPELLMGR)pSpellMgr;
	PSPELL base_spell = spell_mgr->Spells[base_spell_id];
	if (!base_spell) {
		NativeAutoLootTrace("livespell rejected missing base spell: %d", base_spell_id);
		return true;
	}

	PSPELL target_spell = spell_mgr->Spells[spell_id];
	if (!target_spell) {
		target_spell = new SPELL;
		memset(target_spell, 0, sizeof(SPELL));
		spell_mgr->Spells[spell_id] = target_spell;
	}

	*target_spell = *base_spell;
	target_spell->ID = spell_id;

	const std::string name = NativeGetPairValue(payload, "name");
	if (!name.empty()) {
		NativeCopyText(target_spell->Name, name);
	}

	target_spell->Mana = NativeToInt(NativeGetPairValue(payload, "mana"), target_spell->Mana);
	target_spell->CastTime = NativeToInt(NativeGetPairValue(payload, "cast"), target_spell->CastTime);
	target_spell->FizzleTime = NativeToInt(NativeGetPairValue(payload, "recovery"), target_spell->FizzleTime);
	target_spell->RecastTime = NativeToInt(NativeGetPairValue(payload, "recast"), target_spell->RecastTime);
	target_spell->Range = NativeToFloat(NativeGetPairValue(payload, "range"), target_spell->Range);
	target_spell->AERange = NativeToFloat(NativeGetPairValue(payload, "aoe_range"), target_spell->AERange);
	target_spell->SpellIcon = NativeToInt(NativeGetPairValue(payload, "icon"), target_spell->SpellIcon);
	target_spell->BookIcon = NativeToInt(NativeGetPairValue(payload, "book_icon"), target_spell->BookIcon);
	target_spell->TargetType = static_cast<BYTE>(NativeToInt(NativeGetPairValue(payload, "target_type"), target_spell->TargetType));
	target_spell->Resist = static_cast<BYTE>(NativeToInt(NativeGetPairValue(payload, "resist"), target_spell->Resist));
	target_spell->SpellType = static_cast<BYTE>(NativeToInt(NativeGetPairValue(payload, "spell_type"), target_spell->SpellType));

	for (int i = 0; i < 0x0c; ++i) {
		target_spell->Base[i] = 0;
		target_spell->Base2[i] = 0;
		target_spell->Max[i] = 0;
		target_spell->Calc[i] = 100;
		target_spell->Attrib[i] = 254;
	}

	target_spell->Attrib[0] = NativeToInt(NativeGetPairValue(payload, "effect0"), target_spell->Attrib[0]);
	target_spell->Base[0] = NativeToInt(NativeGetPairValue(payload, "base0"), target_spell->Base[0]);
	target_spell->Max[0] = NativeToInt(NativeGetPairValue(payload, "max0"), target_spell->Max[0]);
	target_spell->Calc[0] = NativeToInt(NativeGetPairValue(payload, "calc0"), target_spell->Calc[0]);

	const int level = NativeToInt(NativeGetPairValue(payload, "level"), 1);
	for (int i = 0; i < 0x10; ++i) {
		target_spell->Level[i] = static_cast<BYTE>(level);
	}

	NativeAutoLootTrace(
		"livespell patched id=%d base=%d version=%d name=%s mana=%d cast=%d",
		spell_id,
		base_spell_id,
		version,
		target_spell->Name,
		target_spell->Mana,
		target_spell->CastTime
	);

	char command[128];
	sprintf_s(command, "/say #livespell ack %d %d", spell_id, version);
	NativeAutoLootSendCommand(command);
	return true;
}

static bool NativeAchievementParseTransport(const char* message)
{
	if (!message || !message[0] || !NativeStartsWith(message, "ACH|")) {
		return false;
	}

	if (NativeStartsWith(message, "ACH|window|clear")) {
		gNativeAchievementLoading = true;
		gNativeAchievementCategories.clear();
		gNativeAchievementRows.clear();
		gNativeAchievementObjectives.clear();
		gNativeAchievementSelectedCategory = 0;
		gNativeAchievementSelectedAchievement = 0;
		gNativeAchievementCategoriesDirty = true;
		gNativeAchievementRowsDirty = true;
		gNativeAchievementObjectivesDirty = true;
		gNativeAchievementDetailTitle = "Select an achievement";
		gNativeAchievementDetailDescription.clear();
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->SetStatus("Loading achievements...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|achievements|clear")) {
		gNativeAchievementLoading = true;
		gNativeAchievementRows.clear();
		gNativeAchievementObjectives.clear();
		gNativeAchievementSelectedAchievement = 0;
		gNativeAchievementRowsDirty = true;
		gNativeAchievementObjectivesDirty = true;
		gNativeAchievementDetailTitle = "Select an achievement";
		gNativeAchievementDetailDescription.clear();
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->SetStatus("Loading achievement category...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|objectives|clear")) {
		gNativeAchievementLoading = true;
		gNativeAchievementObjectives.clear();
		gNativeAchievementObjectivesDirty = true;
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->SetStatus("Loading achievement detail...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|status|")) {
		const std::string payload(message + strlen("ACH|status|"));
		gNativeAchievementCompleted = NativeToInt(NativeGetPairValue(payload, "completed"));
		gNativeAchievementTotal = NativeToInt(NativeGetPairValue(payload, "total"));
		gNativeAchievementPoints = NativeToInt(NativeGetPairValue(payload, "points"));
		gNativeAchievementCategoryCount = NativeToInt(NativeGetPairValue(payload, "categories"));
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|category|")) {
		const std::string payload(message + strlen("ACH|category|"));
		NativeAchievementCategoryRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.parent_id = NativeToInt(NativeGetPairValue(payload, "parent"));
		row.name = NativeGetPairValue(payload, "name");
		row.completed = NativeToInt(NativeGetPairValue(payload, "completed"));
		row.total = NativeToInt(NativeGetPairValue(payload, "total"));
		row.points = NativeToInt(NativeGetPairValue(payload, "points"));
		if (row.id > 0) {
			gNativeAchievementCategories.push_back(row);
			gNativeAchievementCategoriesDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|selection|")) {
		const std::string payload(message + strlen("ACH|selection|"));
		gNativeAchievementSelectedCategory = NativeToInt(NativeGetPairValue(payload, "category"));
		gNativeAchievementSelectedAchievement = NativeToInt(NativeGetPairValue(payload, "achievement"));
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|achievement|")) {
		const std::string payload(message + strlen("ACH|achievement|"));
		NativeAchievementRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.category_id = NativeToInt(NativeGetPairValue(payload, "category"));
		row.completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		row.points = NativeToInt(NativeGetPairValue(payload, "points"));
		row.name = NativeGetPairValue(payload, "name");
		row.description = NativeGetPairValue(payload, "description");
		if (row.id > 0) {
			gNativeAchievementRows.push_back(row);
			gNativeAchievementRowsDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|detail|")) {
		const std::string payload(message + strlen("ACH|detail|"));
		gNativeAchievementSelectedAchievement = NativeToInt(NativeGetPairValue(payload, "id"), gNativeAchievementSelectedAchievement);
		const bool completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		const int points = NativeToInt(NativeGetPairValue(payload, "points"));
		const std::string name = NativeGetPairValue(payload, "name");
		const std::string description = NativeGetPairValue(payload, "description");
		char title[160];
		sprintf_s(title, "%s [%s, %d pts]", name.empty() ? "Achievement" : name.c_str(), completed ? "Done" : "Open", points);
		gNativeAchievementDetailTitle = title;
		gNativeAchievementDetailDescription = description;
		gNativeAchievementObjectives.clear();
		gNativeAchievementObjectivesDirty = true;
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|objective|")) {
		const std::string payload(message + strlen("ACH|objective|"));
		NativeAchievementObjectiveRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.completed = NativeToBool(NativeGetPairValue(payload, "completed"));
		row.current = NativeToInt(NativeGetPairValue(payload, "current"));
		row.required = NativeToInt(NativeGetPairValue(payload, "required"));
		row.name = NativeGetPairValue(payload, "name");
		if (row.id > 0) {
			gNativeAchievementObjectives.push_back(row);
			gNativeAchievementObjectivesDirty = true;
		}
		if (gNativeAchievementWnd && !gNativeAchievementLoading) {
			gNativeAchievementWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "ACH|window|show")) {
		gNativeAchievementLoading = false;
		NativeAchievementEnsureWindow(true);
		if (gNativeAchievementWnd) {
			gNativeAchievementWnd->RefreshRows();
			gNativeAchievementWnd->SetStatus("Achievements loaded.");
		}
		return true;
	}

	return true;
}

static bool NativeAutoLootParseTransport(const char* message)
{
	if (!message || !message[0]) {
		return false;
	}

	if (NativeAchievementParseTransport(message)) {
		return true;
	}

	if (NativeStartsWith(message, "LIVESPELL|upsert|")) {
		return NativeApplyLiveSpellPatch(std::string(message + strlen("LIVESPELL|upsert|")));
	}

	if (NativeStartsWith(message, "LIVESPELL|ui|open")) {
		const char* payload = message + strlen("LIVESPELL|ui|open");
		if (*payload == '|') {
			++payload;
		}

		NativeSpellForgeShowWindow(std::string(payload));
		return true;
	}

	if (NativeStartsWith(message, "LIVESPELL|")) {
		return true;
	}

	if (NativeStartsWith(message, "LIVEITEM|ui|open")) {
		const char* payload = message + strlen("LIVEITEM|ui|open");
		if (*payload == '|') {
			++payload;
		}

		NativeItemForgeShowWindow(std::string(payload));
		return true;
	}

	if (NativeStartsWith(message, "LIVEITEM|")) {
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|snapshot|begin") || NativeStartsWith(message, "AUTOLOOT|clear|loot")) {
		gNativeAutoLootRows.clear();
		NativeAutoLootUpdateWindow();
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|snapshot|end")) {
		NativeAutoLootUpdateWindow();
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|window|show")) {
		NativeAutoLootEnsureWindow(true);
		if (gNativeAutoLootWnd) {
			gNativeAutoLootWnd->SetStatus("AutoLoot window reopened.");
		}
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|entry|") || NativeStartsWith(message, "AUTOLOOT|loot|")) {
		const char* payload_start = strchr(message + 9, '|');
		if (!payload_start) {
			return true;
		}

		const std::string payload(payload_start + 1);
		NativeAutoLootRow row;
		row.shared = NativeGetPairValue(payload, "scope") == "shared";
		row.entry_id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.item_id = NativeToInt(NativeGetPairValue(payload, "item_id"), NativeToInt(NativeGetPairValue(payload, "item")));
		row.icon_id = NativeToInt(NativeGetPairValue(payload, "icon"));
		row.qty = NativeToInt(NativeGetPairValue(payload, "qty"), NativeToInt(NativeGetPairValue(payload, "quantity"), 1));
		if (row.qty < 1) {
			row.qty = 1;
		}
		row.locked = NativeToBool(NativeGetPairValue(payload, "locked"));
		row.nodrop = NativeToBool(NativeGetPairValue(payload, "nodrop"));
		row.item = NativeGetPairValue(payload, "name");
		if (row.item.empty()) {
			row.item = NativeGetPairValue(payload, "item_name");
		}
		if (row.item.empty()) {
			char name[64];
			sprintf_s(name, "Item %d", row.item_id);
			row.item = name;
		}
		row.source = NativeGetPairValue(payload, "source");
		if (row.source.empty()) {
			row.source = NativeGetPairValue(payload, "corpse");
		}
		if (row.source.empty()) {
			row.source = "corpse";
		}
		row.state = NativeGetPairValue(payload, "state");
		if (row.state.empty()) {
			row.state = NativeGetPairValue(payload, "status");
		}
		if (row.state.empty()) {
			row.state = "waiting";
		}
		row.rule = NativeGetPairValue(payload, "rule");
		if (row.rule.empty()) {
			row.rule = NativeGetPairValue(payload, "filter");
		}
		if (row.rule.empty()) {
			row.rule = "-";
		}

		if (row.entry_id > 0) {
			gNativeAutoLootRows.push_back(row);
			NativeAutoLootUpdateWindow();
		}
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|status|")) {
		const std::string payload(message + strlen("AUTOLOOT|status|"));
		gNativeAutoLootEnabled = NativeToBool(NativeGetPairValue(payload, "enabled"));
		gNativeAutoLootGrouped = NativeToBool(NativeGetPairValue(payload, "grouped"));
		gNativeAutoLootLeader = NativeToBool(NativeGetPairValue(payload, "leader"));
		gNativeAutoLootKeepCount = NativeToInt(NativeGetPairValue(payload, "include"));
		gNativeAutoLootIgnoreCount = NativeToInt(NativeGetPairValue(payload, "exclude"));
		gNativeAutoLootGroupMode = NativeGetPairValue(payload, "group_mode");
		if (gNativeAutoLootGroupMode.empty()) {
			gNativeAutoLootGroupMode = gNativeAutoLootGrouped ? "group" : "solo";
		}
		gNativeAutoLootAssigned = NativeGetPairValue(payload, "assigned");
		if (gNativeAutoLootAssigned.empty()) {
			gNativeAutoLootAssigned = "none";
		}
		NativeAutoLootUpdateWindow();
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|filters|begin")) {
		gNativeAutoLootRuleRows.clear();
		if (gNativeAutoLootRulesWnd) {
			gNativeAutoLootRulesWnd->RefreshRows();
			gNativeAutoLootRulesWnd->SetStatus("Loading filters...");
		}
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|filter|")) {
		const std::string payload(message + strlen("AUTOLOOT|filter|"));
		NativeAutoLootRuleRow row;
		row.rule = NativeGetPairValue(payload, "mode");
		row.item_id = NativeToInt(NativeGetPairValue(payload, "item_id"));
		row.icon_id = NativeToInt(NativeGetPairValue(payload, "icon"));
		row.item = NativeGetPairValue(payload, "name");
		if (row.item.empty()) {
			char name[64];
			sprintf_s(name, "Item %d", row.item_id);
			row.item = name;
		}

		if (row.item_id > 0) {
			gNativeAutoLootRuleRows.push_back(row);
		}
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|filters|end")) {
		if (gNativeAutoLootRulesWnd) {
			gNativeAutoLootRulesWnd->RefreshRows();
			gNativeAutoLootRulesWnd->SetStatus("Filters refreshed.");
		}
		return true;
	}

	if (NativeStartsWith(message, "AUTOLOOT|filters|")) {
		return true;
	}

	if (strstr(message, "You say, '#autoloot") || strstr(message, "You say, '#lootfilter") || strstr(message, "You say, '#livespell") || strstr(message, "You say, '#itemforge") || strstr(message, "You say, '#ach")) {
		return true;
	}

	return false;
}

class NativeAutoLootChatHook
{
public:
	VOID Trampoline(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst);
	VOID Detour(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst)
	{
		NativeAutoLootMaybeSendInitialRequests();

		if (NativeAutoLootParseTransport(szMsg)) {
			return;
		}

		Trampoline(szMsg, dwColor, EqLog, dopercentsubst);
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID NativeAutoLootChatHook::Trampoline(PCHAR szMsg, DWORD dwColor, bool EqLog, bool dopercentsubst));

static void NativeAutoLootInstallChatHook()
{
	if (gNativeAutoLootChatHookInstalled) {
		return;
	}

	NativeAutoLootTrace("installing chat hook");
	EzDetour(CEverQuest__dsp_chat, &NativeAutoLootChatHook::Detour, &NativeAutoLootChatHook::Trampoline);
	gNativeAutoLootChatHookInstalled = true;
}

static void NativeAutoLootInstallCommandHook()
{
	if (gNativeAutoLootCommandHookInstalled) {
		return;
	}

	if (isMQInjectsEnabled) {
		NativeAutoLootTrace("slash command hook skipped because MQ command hook is enabled");
		return;
	}

	NativeAutoLootTrace("installing slash command hook");
	EzDetour(CEverQuest__InterpretCmd, &NativeAutoLootCommandHook::Detour, &NativeAutoLootCommandHook::Trampoline);
	gNativeAutoLootCommandHookInstalled = true;
}

static void NativeAutoLootResetSessionRequests()
{
	gNativeAutoLootInGamePulses = 0;
	gNativeAutoLootRequestedInitialStatus = false;
	gNativeLiveSpellSentReady = false;
}

static void NativeAutoLootMaybeSendInitialRequests()
{
	const DWORD state = GetGameState();
	if (state != GAMESTATE_INGAME || !pEverQuest || !pLocalPlayer || !GetCharInfo2()) {
		NativeAutoLootResetSessionRequests();
		return;
	}

	if (!gNativeAutoLootRequestedInitialStatus) {
		gNativeAutoLootRequestedInitialStatus = true;
		NativeAutoLootSendCommand("/say #autoloot native status");
	}

	if (!gNativeLiveSpellSentReady) {
		gNativeLiveSpellSentReady = true;
		NativeAutoLootSendCommand("/say #livespell ready");
	}
}

static void NativeAutoLootPulse()
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	const DWORD state = GetGameState();
	if (state != GAMESTATE_INGAME) {
		NativeAutoLootResetSessionRequests();
		return;
	}

	if (!pEverQuest || !pLocalPlayer || !GetCharInfo2()) {
		NativeAutoLootResetSessionRequests();
		return;
	}

	if (gNativeAutoLootInGamePulses < 120) {
		++gNativeAutoLootInGamePulses;
		return;
	}

	if (!gNativeAutoLootWnd) {
		NativeAutoLootTrace("creating window");
		gNativeAutoLootWnd = new NativeAutoLootWnd();
		gNativeAutoLootWnd->RefreshRows();
		NativeAutoLootTrace("window created");
	}

	NativeAutoLootInstallChatHook();

	if (gNativeAutoLootWnd) {
		gNativeAutoLootWnd->Layout();
	}

	if (gNativeAutoLootRulesWnd) {
		gNativeAutoLootRulesWnd->Layout();
	}

	if (gNativeSpellForgeWnd) {
		gNativeSpellForgeWnd->Layout();
	}

	if (gNativeItemForgeWnd) {
		gNativeItemForgeWnd->Layout();
	}

	if (gNativeAchievementWnd) {
		gNativeAchievementWnd->Layout();
	}

	if (!gNativeAutoLootRequestedInitialStatus) {
		gNativeAutoLootRequestedInitialStatus = true;
		NativeAutoLootSendCommand("/say #autoloot native status");
	}

	if (!gNativeLiveSpellSentReady) {
		gNativeLiveSpellSentReady = true;
		NativeAutoLootSendCommand("/say #livespell ready");
	}
}

BOOL NativeAutoLoot_ProcessGameEvents_Trampoline(VOID);
BOOL NativeAutoLoot_ProcessGameEvents_Detour(VOID)
{
	BOOL result = NativeAutoLoot_ProcessGameEvents_Trampoline();
	if (gNativeAutoLootPulseFaulted) {
		return result;
	}

	__try {
		NativeAutoLootPulse();
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		gNativeAutoLootPulseFaulted = true;
		NativeAutoLootTrace("pulse hook faulted; disabling native autoloot pulse for this session");
	}

	return result;
}
DETOUR_TRAMPOLINE_EMPTY(BOOL NativeAutoLoot_ProcessGameEvents_Trampoline(VOID));

static void InitAutoLootNative()
{
	if (gNativeAutoLootHooksInstalled) {
		return;
	}

	gNativeAutoLootHooksInstalled = true;
	NativeAutoLootInstallChatHook();
	NativeAutoLootInstallCommandHook();

	if (!gNativeAutoLootPulseHookEnabled) {
		NativeAutoLootTrace("native pulse hook disabled");
		return;
	}

	NativeAutoLootTrace("installing pulse hook");
	gNativeAutoLootPulseHookInstalled = true;
	EzDetour(ProcessGameEvents, NativeAutoLoot_ProcessGameEvents_Detour, NativeAutoLoot_ProcessGameEvents_Trampoline);
}

static void ShutdownAutoLootNative()
{
	NativeAutoLootTrace("shutdown");

	if (gNativeAutoLootWnd) {
		delete gNativeAutoLootWnd;
		gNativeAutoLootWnd = nullptr;
	}

	if (gNativeAutoLootRulesWnd) {
		delete gNativeAutoLootRulesWnd;
		gNativeAutoLootRulesWnd = nullptr;
	}

	if (gNativeSpellForgeWnd) {
		delete gNativeSpellForgeWnd;
		gNativeSpellForgeWnd = nullptr;
	}

	if (gNativeItemForgeWnd) {
		delete gNativeItemForgeWnd;
		gNativeItemForgeWnd = nullptr;
	}

	if (gNativeAchievementWnd) {
		delete gNativeAchievementWnd;
		gNativeAchievementWnd = nullptr;
	}

	if (gNativeAutoLootChatHookInstalled) {
		RemoveDetour(CEverQuest__dsp_chat);
		gNativeAutoLootChatHookInstalled = false;
	}

	if (gNativeAutoLootCommandHookInstalled) {
		RemoveDetour(CEverQuest__InterpretCmd);
		gNativeAutoLootCommandHookInstalled = false;
	}

	if (gNativeAutoLootPulseHookInstalled) {
		RemoveDetour((DWORD)ProcessGameEvents);
		gNativeAutoLootPulseHookInstalled = false;
	}

	gNativeAutoLootHooksInstalled = false;
	gNativeAutoLootRequestedInitialStatus = false;
	gNativeLiveSpellSentReady = false;
	gNativeAutoLootPulseFaulted = false;
	gNativeAutoLootInGamePulses = 0;
}

#endif
