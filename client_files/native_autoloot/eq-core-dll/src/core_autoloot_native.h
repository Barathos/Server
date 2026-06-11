#ifndef CORE_AUTOLOOT_NATIVE_H
#define CORE_AUTOLOOT_NATIVE_H

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace nativeinterface {
	bool HandleChatMessage(const char* message);
	bool HandleCommandLine(const char* line);
	void Chat(const char* fmt, ...);
	void Start(HMODULE module);
	void Shutdown();
}

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
	std::string status_kind = "waiting";
	std::string vote = "-";
	std::string rule = "-";
	std::string owner;
	std::string assignee;
	std::string master_name;
	std::string lock_reason;
	bool auto_roll = false;
	bool free_grab = false;
	bool eligible = false;
	bool manage = false;
	bool can_loot = false;
	bool can_vote = false;
	bool can_ask = false;
	bool can_roll = false;
	bool can_freegrab = false;
	bool can_give = false;
	bool can_leave = false;
	int roll_seconds = 0;
	int need_count = 0;
	int greed_count = 0;
	int no_count = 0;
	int waiting_count = 0;
};

struct NativeAutoLootManagePlayer
{
	int entry_id = 0;
	int character_id = 0;
	bool master = false;
	bool eligible = false;
	std::string name;
	std::string vote = "waiting";
};

static void NativeAutoLootSendCommand(const char* command);
static void NativeAutoLootUpdateWindow();
static const char* NativeAutoLootToggleEnabledCommand();
static const char* NativeAutoLootToggleApplyFiltersCommand();
static const char* NativeAutoLootToggleMasterCandidateCommand();
static void NativeAutoLootShowRulesWindow();
static void NativeAutoLootShowSettingsWindow();
static void NativeAutoLootShowManageWindow(int entry_id);
static void NativeAutoLootMaybeSendInitialRequests();
static void NativeSpellForgeShowWindow(const std::string& payload);
static void NativeItemForgeShowWindow(const std::string& payload);
static void NativeAutoLootShowManageWindow(int entry_id);
static void NativeAutoLootShowRowMenu(bool shared, int entry_id, int anchor_x, int anchor_y);
static void NativeAutoLootHideRowMenu();
static void NativeAchievementEnsureWindow(bool show);
static bool NativeAchievementParseTransport(const char* message);
static void NativeFactionEnsureWindow(bool show);
static bool NativeFactionParseTransport(const char* message);
static void NativeDpsEnsureWindow(bool show);
static bool NativeDpsParseTransport(const char* message);
static void NativeTradeskillsEnsureWindow(bool show);
static void NativeUIShowcaseEnsureWindow(bool show);
static void NativeHpFixEnsureWindow(bool show);
static void NativeMulticlassEnsureWindow(bool show);
static void NativeMulticlassEnsurePetWindow(bool show);
static void NativeMulticlassEnsureMelodyWindow(bool show);
static void NativeMulticlassEnsureDisciplineWindow(bool show);
static bool NativeMulticlassParseTransport(const char* message);
static void NativeMulticlassInstallContextMenuHook();
static void NativeMulticlassMaintainPresentationUI();
static bool NativeAutoFollowLocalCommand(const char* line);
static void NativeAutoFollowPulse();

static bool gNativeAutoLootGroupByNpcDisplay = false;
static CTextureAnimation* gNativeAutoLootDragItemAnimation = nullptr;
static CTextureAnimation* gNativeAutoLootCheckNormalAnimation = nullptr;
static CTextureAnimation* gNativeAutoLootCheckPressedAnimation = nullptr;
static CTextureAnimation* gNativeAutoLootCloseAnimation = nullptr;

enum NativeAutoLootPersonalColumn
{
	kAALPersonalItem = 0,
	kAALPersonalLoot,
	kAALPersonalLeave,
	kAALPersonalAlwaysNeed,
	kAALPersonalAlwaysGreed,
	kAALPersonalNever,
	kAALPersonalSource
};

enum NativeAutoLootSharedColumn
{
	kAALSharedItem = 0,
	kAALSharedStatus,
	kAALSharedAction,
	kAALSharedManage,
	kAALSharedAutoRoll,
	kAALSharedNeed,
	kAALSharedGreed,
	kAALSharedNo,
	kAALSharedAlwaysNeed,
	kAALSharedAlwaysGreed,
	kAALSharedNever,
	kAALSharedSource
};

static bool NativeAutoLootTryGetClickedCell(CListWnd* list, void* hit_test_point, int* row_index, int* column_index)
{
	if (!list || !hit_test_point || !row_index || !column_index) {
		return false;
	}

	int hit_row = -1;
	int hit_column = -1;

	__try {
		CXPoint point;
		CXPoint* source = (CXPoint*)hit_test_point;
		point.A = source->A;
		point.B = source->B;
		list->GetItemAtPoint(point, &hit_row, &hit_column);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("Advanced Loot list hit test faulted");
		return false;
	}

	if (hit_row < 0 || hit_column < 0) {
		return false;
	}

	*row_index = hit_row;
	*column_index = hit_column;
	return true;
}

static int NativeAutoLootIconCell(int icon_id)
{
	if (icon_id <= 0) {
		return 336;
	}

	return icon_id >= 500 ? icon_id - 500 : icon_id;
}

static CTextureAnimation* NativeAutoLootFindAnimation(const char* name, CTextureAnimation*& cached)
{
	if (cached || !name || !name[0] || !pSidlMgr) {
		return cached;
	}

	CXStr animation_name(name);
	cached = pSidlMgr->FindAnimation(animation_name);
	return cached;
}

static CTextureAnimation* NativeAutoLootDragItemAnimation()
{
	return NativeAutoLootFindAnimation("A_DragItem", gNativeAutoLootDragItemAnimation);
}

static CTextureAnimation* NativeAutoLootActionCellAnimation(bool active, bool negative)
{
	if (negative && active) {
		CTextureAnimation* close_animation = NativeAutoLootFindAnimation("A_CloseBtnNormal", gNativeAutoLootCloseAnimation);
		if (close_animation) {
			return close_animation;
		}
	}

	if (active) {
		return NativeAutoLootFindAnimation("A_CheckBoxPressed", gNativeAutoLootCheckPressedAnimation);
	}

	return NativeAutoLootFindAnimation("A_CheckBoxNormal", gNativeAutoLootCheckNormalAnimation);
}

static const char* NativeAutoLootSquareText(bool active, bool enabled)
{
	if (active) {
		return "[X]";
	}

	return enabled ? "[ ]" : "-";
}

static COLORREF NativeAutoLootSquareColor(bool active, bool enabled, COLORREF active_color)
{
	if (active) {
		return active_color;
	}

	return enabled ? 0xFF8EA8C0 : 0xFF606060;
}

struct NativeAutoLootInlineCellSpec
{
	CListWnd* list;
	int row;
	int column;
	bool active;
	bool enabled;
	bool negative;
};

static std::vector<NativeAutoLootInlineCellSpec> gNativeAutoLootInlineCellSpecs;

// Diagnostic counters for the inline-cell work, surfaced via the status label
// and native_autoloot.log. Remove once the inline UI is confirmed in-game.
static unsigned int gNativeAutoLootDiagPulseCount = 0;
static unsigned int gNativeAutoLootDiagPostDrawCount = 0;
static unsigned int gNativeAutoLootDiagOnProcessFrameCount = 0;
static DWORD gNativeAutoLootDiagLastReportTick = 0;
static int gNativeAutoLootDiagReportBudget = 300;
static int gNativeAutoLootDiagNotifyBudget = 120;
static int gNativeAutoLootDiagStructDumpBudget = 1;

// CButtonWnd keeps its NormalDecal animation instance at +0x240 (verified
// via the 2026-06-09 struct dumps: decal-less templates read null there,
// decal templates read a heap animation pointer).
static const int kAALButtonNormalDecalOffset = 0x240;
static bool gNativeAutoLootIconCellFaulted = false;

// One-shot struct dump for the item-icon investigation: locate the decal
// animation pointer inside CButtonWnd by comparing differently-templated
// buttons. Read-only and SEH-guarded.
static void NativeAutoLootDiagDumpButton(const char* tag, CButtonWnd* button)
{
	if (!button) {
		NativeAutoLootTrace("DIAG dump %s: null", tag);
		return;
	}

	DWORD* raw = (DWORD*)button;
	__try {
		for (int base = 0x1C0; base < 0x320; base += 0x20) {
			char line[256];
			sprintf_s(line, "DIAG dump %s +%03X: %08X %08X %08X %08X %08X %08X %08X %08X",
				tag, base,
				raw[base / 4 + 0], raw[base / 4 + 1], raw[base / 4 + 2], raw[base / 4 + 3],
				raw[base / 4 + 4], raw[base / 4 + 5], raw[base / 4 + 6], raw[base / 4 + 7]);
			NativeAutoLootTrace("%s", line);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("DIAG dump %s faulted", tag);
	}
}

// Inline checkbox pool: real XML-declared checkbox controls positioned over
// the list cells every pulse. Uses only primitives proven to work in this
// client (GetChildItem, GetItemRect, Show, direct Location/Checked struct
// writes). The FindAnimation, DrawColoredRect, and CreateXWndFromTemplate
// paths all fault in this client per the 2026-06-09 diagnostic log.
static const int kAALPoolPersonalRows = 8;
static const int kAALPoolPersonalCols = 5;
static const int kAALPoolSharedRows = 14;
static const int kAALPoolSharedCols = 10;
static const int kAALRulePoolRows = 24;

static const int kAALPoolPersonalColumns[kAALPoolPersonalCols] = {
	kAALPersonalLoot, kAALPersonalLeave, kAALPersonalAlwaysNeed, kAALPersonalAlwaysGreed, kAALPersonalNever
};

static const int kAALPoolSharedColumns[kAALPoolSharedCols] = {
	kAALSharedStatus, kAALSharedAction, kAALSharedManage, kAALSharedAutoRoll, kAALSharedNeed, kAALSharedGreed, kAALSharedNo, kAALSharedAlwaysNeed, kAALSharedAlwaysGreed, kAALSharedNever
};

// Icon-art buttons (Live's 36x36 sprites scaled down) get a slightly larger
// footprint than the plain 16x16 checkboxes.
static int NativeAutoLootPoolControlSize(bool shared, int column)
{
	if (!shared) {
		return (column == kAALPersonalLoot || column == kAALPersonalLeave || column == kAALPersonalNever) ? 26 : 24;
	}

	return (column == kAALSharedStatus || column == kAALSharedAction || column == kAALSharedManage) ? 26 : 24;
}

static int NativeAutoLootPoolSlotForColumn(bool shared, int column)
{
	const int* columns = shared ? kAALPoolSharedColumns : kAALPoolPersonalColumns;
	const int count = shared ? kAALPoolSharedCols : kAALPoolPersonalCols;
	for (int i = 0; i < count; ++i) {
		if (columns[i] == column) {
			return i;
		}
	}

	return -1;
}

struct NativeAutoLootPoolCellState
{
	CButtonWnd* button;
	CListWnd* list;
	int row;
	int column;
	bool enabled;
	int intended_left;
	int intended_top;
};

// Shared cell-rect computation for overlay controls: returns the screen-space
// rect for a control of the given size inside the cell, fully clipped to the
// list's visible area.
// CListWnd::GetItemRect returns list-relative rects in this client. The old
// per-cell guess (cell left/top < list origin - 4) misread right-hand columns
// of lower rows (large relative X/Y) as absolute, scattering pool controls
// when the list scrolled or the window sat high on screen. Column 0's left
// edge is ~0 in relative mode regardless of scroll position, so probe that.
static bool NativeAutoLootListRectIsRelative(CListWnd* list, int row, int column, const CXRect& cell_rect, const CXRect& list_rect)
{
	if (column == 0) {
		return (int)cell_rect.A < (int)list_rect.A - 4;
	}

	CXRect probe = list->GetItemRect(row, 0);
	return (int)probe.A < (int)list_rect.A - 4;
}

static bool NativeAutoLootCellControlRect(CListWnd* list, int row, int column, int control_size, bool left_align, CXRect* target, CXRect* clip)
{
	if (!list || !target || !clip) {
		return false;
	}

	__try {
		CXRect cell_rect = list->GetItemRect(row, column);
		CXRect list_rect = ((CXWnd*)list)->GetScreenRect();
		CXRect list_clip = ((CXWnd*)list)->GetScreenClipRect();

		int left = (int)cell_rect.A;
		int top = (int)cell_rect.B;
		int right = (int)cell_rect.C;
		int bottom = (int)cell_rect.D;

		if (NativeAutoLootListRectIsRelative(list, row, column, cell_rect, list_rect)) {
			left += (int)list_rect.A;
			right += (int)list_rect.A;
			top += (int)list_rect.B;
			bottom += (int)list_rect.B;
		}

		if (right <= left || bottom <= top ||
			right <= (int)list_clip.A || left >= (int)list_clip.C ||
			bottom <= (int)list_clip.B || top >= (int)list_clip.D) {
			return false;
		}

		int horizontal_inset = left_align ? 2 : (right - left - control_size) / 2;
		int vertical_inset = (bottom - top - control_size) / 2;
		if (horizontal_inset < 0) {
			horizontal_inset = 0;
		}
		if (vertical_inset < 0) {
			vertical_inset = 0;
		}

		target->A = left + horizontal_inset;
		target->B = top + vertical_inset;
		target->C = left + horizontal_inset + control_size;
		target->D = top + vertical_inset + control_size;
		clip->A = list_clip.A;
		clip->B = list_clip.B;
		clip->C = list_clip.C;
		clip->D = list_clip.D;

		if ((int)target->B < (int)clip->B || (int)target->D > (int)clip->D) {
			return false;
		}

		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

static void NativeAutoLootSetSquareCell(CListWnd* list, int row, int column, bool active, bool enabled, bool negative)
{
	if (!list) {
		return;
	}

	NativeAutoLootInlineCellSpec spec = { list, row, column, active, enabled, negative };
	gNativeAutoLootInlineCellSpecs.push_back(spec);

	CXStr spacer(" ");
	list->SetItemText(row, column, &spacer);
}

static bool NativeAutoLootIsNeedVote(const NativeAutoLootRow& row)
{
	return row.vote == "need" || row.state == "need" || row.state == "alwaysneed";
}

static bool NativeAutoLootIsGreedVote(const NativeAutoLootRow& row)
{
	return row.vote == "greed" || row.state == "greed" || row.state == "alwaysgreed";
}

static bool NativeAutoLootIsNoVote(const NativeAutoLootRow& row)
{
	return row.vote == "pass" || row.vote == "no" || row.state == "pass" || row.state == "no";
}

static void NativeAutoLootSetColumnJustification(CListWnd* list, int first_column, int last_column, int justification)
{
	if (!list) {
		return;
	}

	__try {
		for (int column = first_column; column <= last_column; ++column) {
			list->SetColumnJustification(column, justification);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("Advanced Loot column justification faulted");
	}
}

static int NativeAutoLootGetListWidth(CListWnd* list)
{
	if (!list) {
		return 0;
	}

	__try {
		CXRect rect = ((CXWnd*)list)->GetScreenRect();
		return (int)rect.C - (int)rect.A;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("Advanced Loot list width faulted");
		return 0;
	}
}

static void NativeAutoLootSetColumnWidth(CListWnd* list, int column, int width)
{
	if (!list || width <= 0) {
		return;
	}

	__try {
		list->SetColumnWidth(column, width);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("Advanced Loot column width faulted");
	}
}

static void NativeAutoLootFitListColumns(CListWnd* list, bool shared)
{
	const int width = NativeAutoLootGetListWidth(list);
	if (width <= 0) {
		return;
	}

	const int usable_width = width > 28 ? width - 28 : 0;
	if (!shared) {
		const int base_total = 210 + 56 + 60 + 46 + 46 + 64 + 280;
		const int extra = usable_width > base_total ? usable_width - base_total : 0;
		const int item_width = 210 + (extra * 45 / 100);
		const int source_width = 280 + (extra - (extra * 45 / 100));

		NativeAutoLootSetColumnWidth(list, kAALPersonalItem, item_width);
		NativeAutoLootSetColumnWidth(list, kAALPersonalLoot, 56);
		NativeAutoLootSetColumnWidth(list, kAALPersonalLeave, 60);
		NativeAutoLootSetColumnWidth(list, kAALPersonalAlwaysNeed, 46);
		NativeAutoLootSetColumnWidth(list, kAALPersonalAlwaysGreed, 46);
		NativeAutoLootSetColumnWidth(list, kAALPersonalNever, 64);
		NativeAutoLootSetColumnWidth(list, kAALPersonalSource, source_width);
		return;
	}

	const int base_total = 160 + 88 + 64 + 52 + 44 + 44 + 44 + 44 + 44 + 44 + 44 + 170;
	const int extra = usable_width > base_total ? usable_width - base_total : 0;
	const int item_extra = extra * 40 / 100;
	const int source_extra = extra * 40 / 100;
	const int item_width = 160 + item_extra;
	const int source_width = 170 + source_extra;
	const int status_width = 88 + (extra - item_extra - source_extra);

	NativeAutoLootSetColumnWidth(list, kAALSharedItem, item_width);
	NativeAutoLootSetColumnWidth(list, kAALSharedStatus, status_width);
	NativeAutoLootSetColumnWidth(list, kAALSharedAction, 64);
	NativeAutoLootSetColumnWidth(list, kAALSharedManage, 52);
	NativeAutoLootSetColumnWidth(list, kAALSharedAutoRoll, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedNeed, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedGreed, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedNo, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedAlwaysNeed, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedAlwaysGreed, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedNever, 44);
	NativeAutoLootSetColumnWidth(list, kAALSharedSource, source_width);
}

class NativeAutoLootWnd : public CCustomWnd
{
public:
	NativeAutoLootWnd() : CCustomWnd((char*)"NativeAutoLootWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAutoLootWnd);
		int (NativeAutoLootWnd::*pfOnProcessFrame)() = &NativeAutoLootWnd::OnProcessFrame;
		SetvfTable(49, *(DWORD*)&pfOnProcessFrame);
		int (NativeAutoLootWnd::*pfPostDraw)() const = &NativeAutoLootWnd::PostDraw;
		SetvfTable(3, *(DWORD*)&pfPostDraw);

		PersonalLabel = GetChildItem("AALW_PersonalLabel");
		SetAllLabel = GetChildItem("AALW_SetAllLabel");
		SharedLabel = GetChildItem("AALW_SharedLabel");
		PersonalList = (CListWnd*)GetChildItem("AALW_PersonalList");
		SharedList = (CListWnd*)GetChildItem("AALW_SharedList");
		StatusLabel = GetChildItem("AALW_StatusLabel");
		MasterLabel = GetChildItem("AALW_MasterLabel");
		RuleSummaryLabel = GetChildItem("AALW_RuleSummaryLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("AALW_RefreshButton");
		EditFiltersButton = (CButtonWnd*)GetChildItem("AALW_EditFiltersButton");
		LootSettingsButton = (CButtonWnd*)GetChildItem("AALW_LootSettingsButton");
		LootAllButton = (CButtonWnd*)GetChildItem("AALW_LootAllButton");
		LeaveAllButton = (CButtonWnd*)GetChildItem("AALW_LeaveAllButton");
		LootButton = (CButtonWnd*)GetChildItem("AALW_LootButton");
		LeaveButton = (CButtonWnd*)GetChildItem("AALW_LeaveButton");
		AlwaysButton = (CButtonWnd*)GetChildItem("AALW_AlwaysButton");
		NeverButton = (CButtonWnd*)GetChildItem("AALW_NeverButton");
		NeedButton = (CButtonWnd*)GetChildItem("AALW_NeedButton");
		GreedButton = (CButtonWnd*)GetChildItem("AALW_GreedButton");
		NoButton = (CButtonWnd*)GetChildItem("AALW_NoButton");
		AlwaysNeedButton = (CButtonWnd*)GetChildItem("AALW_AlwaysNeedButton");
		AlwaysGreedButton = (CButtonWnd*)GetChildItem("AALW_AlwaysGreedButton");
		AskButton = (CButtonWnd*)GetChildItem("AALW_AskButton");
		RollButton = (CButtonWnd*)GetChildItem("AALW_RollButton");
		FreeGrabButton = (CButtonWnd*)GetChildItem("AALW_FreeGrabButton");
		GiveButton = (CButtonWnd*)GetChildItem("AALW_GiveButton");
		ManageButton = (CButtonWnd*)GetChildItem("AALW_ManageButton");
		LeaveCorpseButton = (CButtonWnd*)GetChildItem("AALW_LeaveCorpseButton");
		ApplyFiltersCheck = (CButtonWnd*)GetChildItem("AALW_ApplyFiltersCheck");
		GroupedByNpcCheck = (CButtonWnd*)GetChildItem("AALW_GroupedByNpcCheck");
		PersonalSetCombo = (CComboWnd*)GetChildItem("AALW_PersonalSetCombo");
		SharedSetCombo = (CComboWnd*)GetChildItem("AALW_SharedSetCombo");
		SharedLeaveAllButton = (CButtonWnd*)GetChildItem("AALW_SharedLeaveAllButton");

		for (int pool_row = 0; pool_row < kAALPoolPersonalRows; ++pool_row) {
			char name[48];
			sprintf_s(name, "AALW_IBP_R%d", pool_row);
			PersonalIconPool[pool_row] = (CButtonWnd*)GetChildItem(name);
			if (PersonalIconPool[pool_row]) {
				((CXWnd*)PersonalIconPool[pool_row])->Show(0, 1);
			}
		}

		for (int pool_row = 0; pool_row < kAALPoolSharedRows; ++pool_row) {
			char name[48];
			sprintf_s(name, "AALW_IBS_R%d", pool_row);
			SharedIconPool[pool_row] = (CButtonWnd*)GetChildItem(name);
			if (SharedIconPool[pool_row]) {
				((CXWnd*)SharedIconPool[pool_row])->Show(0, 1);
			}
		}
		NativeAutoLootTrace("DIAG ptrs plist=%p slist=%p pcombo=%p scombo=%p sleave=%p",
			PersonalList, SharedList, PersonalSetCombo, SharedSetCombo, SharedLeaveAllButton);

		for (int pool_row = 0; pool_row < kAALPoolPersonalRows; ++pool_row) {
			for (int slot = 0; slot < kAALPoolPersonalCols; ++slot) {
				char name[48];
				sprintf_s(name, "AALW_CBP_R%dC%d", pool_row, slot);
				PersonalPool[pool_row][slot] = (CButtonWnd*)GetChildItem(name);
				if (PersonalPool[pool_row][slot]) {
					((CXWnd*)PersonalPool[pool_row][slot])->Show(0, 1);
				}
			}
		}

		for (int pool_row = 0; pool_row < kAALPoolSharedRows; ++pool_row) {
			for (int slot = 0; slot < kAALPoolSharedCols; ++slot) {
				char name[48];
				sprintf_s(name, "AALW_CBS_R%dC%d", pool_row, slot);
				SharedPool[pool_row][slot] = (CButtonWnd*)GetChildItem(name);
				if (SharedPool[pool_row][slot]) {
					((CXWnd*)SharedPool[pool_row][slot])->Show(0, 1);
				}
			}
		}

		NativeAutoLootSetColumnJustification(PersonalList, kAALPersonalLoot, kAALPersonalNever, 1);
		NativeAutoLootSetColumnJustification(SharedList, kAALSharedStatus, kAALSharedNever, 1);
		Layout();
		SetStatus("Waiting for Advanced Loot snapshot...");
		RefreshRows();
	}

	int OnProcessFrame()
	{
		++gNativeAutoLootDiagOnProcessFrameCount;
		return 1;
	}

	int PostDraw() const
	{
		++gNativeAutoLootDiagPostDrawCount;
		return 1;
	}

	void DiagnosticPulse();

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (gNativeAutoLootDiagNotifyBudget > 0 &&
			Message != XWM_LCLICK && Message != XWM_RCLICK &&
			Message != XWM_MOUSEOVER && Message != XWM_CLOSE) {
			--gNativeAutoLootDiagNotifyBudget;
			NativeAutoLootTrace("DIAG notify pWnd=%p msg=%u data=%d", pWnd, Message, (int)(intptr_t)unknown);
		}

		// Combo selection in this client notifies with message 33 (msg 32
		// fires on dropdown open/close), confirmed via the notify log.
		if (Message == XWM_NEWVALUE || Message == 33) {
			if (pWnd == (CXWnd*)PersonalSetCombo) {
				ApplySetAll(false, ResolveComboChoice(PersonalSetCombo, (int)(intptr_t)unknown));
				return 1;
			}

			if (pWnd == (CXWnd*)SharedSetCombo) {
				ApplySetAll(true, ResolveComboChoice(SharedSetCombo, (int)(intptr_t)unknown));
				return 1;
			}
		}

		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_RCLICK && (pWnd == (CXWnd*)PersonalList || pWnd == (CXWnd*)SharedList)) {
			CListWnd* list = (CListWnd*)pWnd;
			ActiveList = list;
			int selected = list ? list->GetCurSel() : -1;
			int column = -1;
			if (list && NativeAutoLootTryGetClickedCell(list, unknown, &selected, &column)) {
				list->SetCurSel(selected);
			}

			NativeAutoLootRow* row = GetSelectedRowFromList(list);
			if (!row) {
				SetStatus("Select a real loot row first.");
				return 1;
			}

			CXRect cell_rect = list->GetItemRect(selected, column >= 0 ? column : 0);
			CXRect list_rect = ((CXWnd*)list)->GetScreenRect();
			int anchor_x = (int)cell_rect.A;
			int anchor_y = (int)cell_rect.D;
			if (NativeAutoLootListRectIsRelative(list, selected, column >= 0 ? column : 0, cell_rect, list_rect)) {
				anchor_x += (int)list_rect.A;
				anchor_y += (int)list_rect.B;
			}

			NativeAutoLootShowRowMenu(list == SharedList, row->entry_id, anchor_x, anchor_y);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			NativeAutoLootHideRowMenu();

			for (const NativeAutoLootPoolCellState& state : PoolStates) {
				if ((CXWnd*)state.button != pWnd) {
					continue;
				}

				if (!state.list) {
					return 1;
				}

				ActiveList = state.list;
				state.list->SetCurSel(state.row);
				HandleListColumnAction(state.list, state.list == SharedList, state.row, state.column);
				return 1;
			}

			// Fallback: route clicks on pool buttons that were missing from
			// PoolStates (e.g. a stale frame between data refresh and sync)
			// by button identity, and log them so dead clicks are visible
			// in native_autoloot.log.
			for (int pool_row = 0; pool_row < kAALPoolPersonalRows; ++pool_row) {
				for (int slot = 0; slot < kAALPoolPersonalCols; ++slot) {
					if (pWnd != (CXWnd*)PersonalPool[pool_row][slot] || !PersonalPool[pool_row][slot]) {
						continue;
					}

					const int list_row = PersonalPoolListRow[pool_row];
					NativeAutoLootTrace("DIAG pool fallback personal r=%d slot=%d listrow=%d", pool_row, slot, list_row);
					if (list_row >= 0 && PersonalList) {
						ActiveList = PersonalList;
						PersonalList->SetCurSel(list_row);
						HandleListColumnAction(PersonalList, false, list_row, kAALPoolPersonalColumns[slot]);
					}
					return 1;
				}
			}

			for (int pool_row = 0; pool_row < kAALPoolSharedRows; ++pool_row) {
				for (int slot = 0; slot < kAALPoolSharedCols; ++slot) {
					if (pWnd != (CXWnd*)SharedPool[pool_row][slot] || !SharedPool[pool_row][slot]) {
						continue;
					}

					const int list_row = SharedPoolListRow[pool_row];
					NativeAutoLootTrace("DIAG pool fallback shared r=%d slot=%d listrow=%d", pool_row, slot, list_row);
					if (list_row >= 0 && SharedList) {
						ActiveList = SharedList;
						SharedList->SetCurSel(list_row);
						HandleListColumnAction(SharedList, true, list_row, kAALPoolSharedColumns[slot]);
					}
					return 1;
				}
			}

			if (pWnd == (CXWnd*)PersonalList || pWnd == (CXWnd*)SharedList) {
				CListWnd* list = (CListWnd*)pWnd;
				ActiveList = list;
				if (HandleListColumnClick(list, list == SharedList, unknown)) {
					return 1;
				}
				return 1;
			}

			if (pWnd == (CXWnd*)RefreshButton) {
				NativeAutoLootSendCommand("/say #advloot native status");
				SetStatus("Refreshing Advanced Loot...");
				return 1;
			}

			if (pWnd == (CXWnd*)EditFiltersButton) {
				NativeAutoLootShowRulesWindow();
				SetStatus("Opened Advanced Loot filters.");
				return 1;
			}

			if (pWnd == (CXWnd*)LootSettingsButton) {
				NativeAutoLootShowSettingsWindow();
				SetStatus("Opened Advanced Loot settings.");
				return 1;
			}

			if (pWnd == (CXWnd*)LootAllButton) {
				NativeAutoLootSendCommand("/say #advloot personal lootall");
				SetStatus("Requested Loot All.");
				return 1;
			}

			if (pWnd == (CXWnd*)LeaveAllButton) {
				NativeAutoLootSendCommand("/say #advloot personal leaveall");
				SetStatus("Requested Leave All.");
				return 1;
			}

			if (pWnd == (CXWnd*)SharedLeaveAllButton) {
				ApplySetAll(true, 6);
				return 1;
			}

			if (pWnd == (CXWnd*)ApplyFiltersCheck) {
				NativeAutoLootSendCommand(NativeAutoLootToggleApplyFiltersCommand());
				SetStatus("Toggled Apply Filters.");
				return 1;
			}

			if (pWnd == (CXWnd*)GroupedByNpcCheck) {
				gNativeAutoLootGroupByNpcDisplay = GroupedByNpcCheck && GroupedByNpcCheck->Checked;
				RefreshRows();
				SetStatus(gNativeAutoLootGroupByNpcDisplay ? "Grouped rows by NPC." : "Using server row order.");
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
	NativeAutoLootRow* GetSelectedRowFromList(CListWnd* list);
	void RefreshList(CListWnd* list, bool shared);
	bool HandleListColumnClick(CListWnd* list, bool shared, void* hit_test_point);
	bool HandleListColumnAction(CListWnd* list, bool shared, int selected, int column);
	bool SendListAction(const NativeAutoLootRow& row, const char* action, const char* status);
	bool SendCorpseAction(const NativeAutoLootRow& row, const char* action, const char* status);
	bool ToggleAutoRollFilter(const NativeAutoLootRow& row);
	bool GetInlineCellDrawRect(const NativeAutoLootInlineCellSpec& spec, CXRect* target, CXRect* clip) const;
	bool GetIconCellDrawRect(CListWnd* list, int row, CXRect* target, CXRect* clip) const;
	void SetIconButtonCell(CButtonWnd* button, int* last_cell, int icon_id);
	void ApplyListRowHeights();
	void SyncInlinePool();
	void ApplySetAll(bool shared, int choice);
	int ResolveComboChoice(CComboWnd* combo, int hint);
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
	CButtonWnd* EditFiltersButton = nullptr;
	CButtonWnd* LootSettingsButton = nullptr;
	CButtonWnd* LootAllButton = nullptr;
	CButtonWnd* LeaveAllButton = nullptr;
	CButtonWnd* LootButton = nullptr;
	CButtonWnd* LeaveButton = nullptr;
	CButtonWnd* AlwaysButton = nullptr;
	CButtonWnd* NeverButton = nullptr;
	CButtonWnd* NeedButton = nullptr;
	CButtonWnd* GreedButton = nullptr;
	CButtonWnd* NoButton = nullptr;
	CButtonWnd* AlwaysNeedButton = nullptr;
	CButtonWnd* AlwaysGreedButton = nullptr;
	CButtonWnd* AskButton = nullptr;
	CButtonWnd* RollButton = nullptr;
	CButtonWnd* FreeGrabButton = nullptr;
	CButtonWnd* GiveButton = nullptr;
	CButtonWnd* ManageButton = nullptr;
	CButtonWnd* LeaveCorpseButton = nullptr;
	CButtonWnd* ApplyFiltersCheck = nullptr;
	CButtonWnd* GroupedByNpcCheck = nullptr;
	CComboWnd* PersonalSetCombo = nullptr;
	CComboWnd* SharedSetCombo = nullptr;
	CButtonWnd* SharedLeaveAllButton = nullptr;
	CButtonWnd* PersonalIconPool[kAALPoolPersonalRows] = {};
	CButtonWnd* SharedIconPool[kAALPoolSharedRows] = {};
	bool PersonalIconShown[kAALPoolPersonalRows] = {};
	bool SharedIconShown[kAALPoolSharedRows] = {};
	int PersonalIconCell[kAALPoolPersonalRows] = {};
	int SharedIconCell[kAALPoolSharedRows] = {};
	int PersonalPoolListRow[kAALPoolPersonalRows] = {};
	int SharedPoolListRow[kAALPoolSharedRows] = {};
	CButtonWnd* PersonalPool[kAALPoolPersonalRows][kAALPoolPersonalCols] = {};
	CButtonWnd* SharedPool[kAALPoolSharedRows][kAALPoolSharedCols] = {};
	bool PersonalPoolShown[kAALPoolPersonalRows][kAALPoolPersonalCols] = {};
	bool SharedPoolShown[kAALPoolSharedRows][kAALPoolSharedCols] = {};
	std::vector<NativeAutoLootPoolCellState> PoolStates;
	std::vector<CButtonWnd*> PoolRefresh;
	bool PoolCalibrated = false;
	int PoolCorrectionX = 0;
	int PoolCorrectionY = 0;
	int LastLayoutWidth = 0;
	int LastLayoutHeight = 0;
};

static NativeAutoLootWnd* gNativeAutoLootWnd = nullptr;
static std::vector<NativeAutoLootRow> gNativeAutoLootRows;

static int NativeAutoLootIconIdForEntry(int entry_id)
{
	if (entry_id <= 0) {
		return 0;
	}

	for (const NativeAutoLootRow& row : gNativeAutoLootRows) {
		if (row.entry_id == entry_id) {
			return row.icon_id;
		}
	}

	return 0;
}
static bool gNativeAutoLootHooksInstalled = false;
static bool gNativeAutoLootChatHookInstalled = false;
static bool gNativeAutoLootCommandHookInstalled = false;
static bool gNativeAutoLootPulseHookInstalled = false;
static bool gNativeAutoLootUiResetHookInstalled = false;
static bool gNativeChatTimestampConfigLoaded = false;
static bool gNativeChatTimestampEnabled = false;
static bool gNativeAutoLootRequestedInitialStatus = false;
static bool gNativeAutoLootPulseHookEnabled = true;
static bool gNativeAutoLootWasInGame = false;
static bool gNativeAutoLootWindowConstructionFaulted = false;
static bool gNativeAutoLootEnabled = false;
static bool gNativeAutoLootApplyFilters = true;
static bool gNativeAutoLootGrouped = false;
static bool gNativeAutoLootLeader = false;
static bool gNativeAutoLootMasterCandidate = true;
static bool gNativeAutoLootAutoSplit = true;
static bool gNativeAutoLootAutoLootAll = false;
static bool gNativeAutoLootAutoShow = true;
static bool gNativeAutoLootShowNewOnly = true;
static bool gNativeAutoLootConfirmRemove = true;
static bool gNativeAutoLootAutoRemoveLore = true;
static bool gNativeAutoLootDebug = false;
static bool gNativeAutoLootLog = false;
static int gNativeAutoLootMasterCharacterId = 0;
static std::string gNativeAutoLootMasterName;
static int gNativeAutoLootInGamePulses = 0;
static int gNativeAutoLootAlwaysNeedCount = 0;
static int gNativeAutoLootAlwaysGreedCount = 0;
static int gNativeAutoLootNeverCount = 0;
static int gNativeAutoLootAutoRollCount = 0;
static bool gNativeLiveSpellSentReady = false;
static bool gNativeHpFixSentReady = false;
static int gNativeHpFixReadyRetryPulses = 0;
static int gNativeHpFixRefreshPulses = 0;
static bool gNativeAutoLootPulseFaulted = false;

struct NativeAutoLootRuleRow
{
	int item_id = 0;
	int icon_id = 0;
	bool auto_roll = false;
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
		ShareButton = (CButtonWnd*)GetChildItem("AALR_ShareButton");
		TglEnabled = (CButtonWnd*)GetChildItem("AALR_TglEnabled");
		TglSplit = (CButtonWnd*)GetChildItem("AALR_TglSplit");
		TglConfirm = (CButtonWnd*)GetChildItem("AALR_TglConfirm");
		TglLore = (CButtonWnd*)GetChildItem("AALR_TglLore");
		TglShow = (CButtonWnd*)GetChildItem("AALR_TglShow");
		TglLootAll = (CButtonWnd*)GetChildItem("AALR_TglLootAll");

		for (int pool_row = 0; pool_row < kAALRulePoolRows; ++pool_row) {
			char name[48];
			for (int slot = 0; slot < 4; ++slot) {
				sprintf_s(name, "AALR_CB_R%dC%d", pool_row, slot);
				CheckPool[pool_row][slot] = (CButtonWnd*)GetChildItem(name);
				if (CheckPool[pool_row][slot]) {
					((CXWnd*)CheckPool[pool_row][slot])->Show(0, 1);
				}
			}

			sprintf_s(name, "AALR_RM_R%d", pool_row);
			RemovePool[pool_row] = (CButtonWnd*)GetChildItem(name);
			if (RemovePool[pool_row]) {
				((CXWnd*)RemovePool[pool_row])->Show(0, 1);
			}

			sprintf_s(name, "AALR_IB_R%d", pool_row);
			IconPool[pool_row] = (CButtonWnd*)GetChildItem(name);
			if (IconPool[pool_row]) {
				((CXWnd*)IconPool[pool_row])->Show(0, 1);
			}

			RulePoolListRow[pool_row] = -1;
		}

		NativeAutoLootSetColumnJustification(RuleList, 2, 6, 1);
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
				NativeAutoLootSendCommand("/say #advloot filter native list");
				SetStatus("Refreshing filters...");
				return 1;
			}

			if (pWnd == (CXWnd*)ShareButton) {
				NativeAutoLootSendCommand("/say #advloot filter share");
				SetStatus("Offering your filters to your target...");
				return 1;
			}

			if (pWnd == (CXWnd*)TglEnabled) {
				NativeAutoLootSendCommand(NativeAutoLootToggleEnabledCommand());
				SetStatus("Toggled Advanced Loot.");
				return 1;
			}

			if (pWnd == (CXWnd*)TglSplit) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoSplit ? "/say #advloot autosplit off" : "/say #advloot autosplit on");
				SetStatus("Toggled Auto Split Coin.");
				return 1;
			}

			if (pWnd == (CXWnd*)TglConfirm) {
				NativeAutoLootSendCommand(gNativeAutoLootConfirmRemove ? "/say #advloot confirmremove off" : "/say #advloot confirmremove on");
				SetStatus("Toggled Confirm Remove.");
				return 1;
			}

			if (pWnd == (CXWnd*)TglLore) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoRemoveLore ? "/say #advloot autoremovelore off" : "/say #advloot autoremovelore on");
				SetStatus("Toggled Auto Remove Lore.");
				return 1;
			}

			if (pWnd == (CXWnd*)TglShow) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoShow ? "/say #advloot autoshow off" : "/say #advloot autoshow on");
				SetStatus("Toggled Auto Show.");
				return 1;
			}

			if (pWnd == (CXWnd*)TglLootAll) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoLootAll ? "/say #advloot autolootall off" : "/say #advloot autolootall on");
				SetStatus("Toggled Auto Loot All.");
				return 1;
			}

			for (int pool_row = 0; pool_row < kAALRulePoolRows; ++pool_row) {
				for (int slot = 0; slot < 4; ++slot) {
					if (pWnd == (CXWnd*)CheckPool[pool_row][slot]) {
						HandleRuleCell(pool_row, slot);
						return 1;
					}
				}

				if (pWnd == (CXWnd*)RemovePool[pool_row]) {
					HandleRuleCell(pool_row, 4);
					return 1;
				}

				if (pWnd == (CXWnd*)IconPool[pool_row]) {
					return 1;
				}
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
	void HandleRuleCell(int pool_row, int slot);
	void SetLabel(CXWnd* label, const char* text);
	void SetButtonCheck(CButtonWnd* button, bool checked)
	{
		if (button) {
			button->Checked = checked ? 1 : 0;
		}
	}

	CXWnd* SummaryLabel = nullptr;
	CXWnd* StatusLabel = nullptr;
	CListWnd* RuleList = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* ShareButton = nullptr;
	CButtonWnd* TglEnabled = nullptr;
	CButtonWnd* TglSplit = nullptr;
	CButtonWnd* TglConfirm = nullptr;
	CButtonWnd* TglLore = nullptr;
	CButtonWnd* TglShow = nullptr;
	CButtonWnd* TglLootAll = nullptr;
	CButtonWnd* CheckPool[kAALRulePoolRows][4] = {};
	CButtonWnd* RemovePool[kAALRulePoolRows] = {};
	CButtonWnd* IconPool[kAALRulePoolRows] = {};
	bool CheckShown[kAALRulePoolRows][4] = {};
	bool RemoveShown[kAALRulePoolRows] = {};
	bool IconShown[kAALRulePoolRows] = {};
	int IconCell[kAALRulePoolRows] = {};
	int RulePoolListRow[kAALRulePoolRows] = {};
	std::vector<int> RowIconIds;
	std::vector<CButtonWnd*> RulesRefresh;
	int RowCount = 0;
	bool PoolCalibrated = false;
	int PoolCorrectionX = 0;
	int PoolCorrectionY = 0;
	CButtonWnd* CalibButton = nullptr;
	int CalibIntendedX = 0;
	int CalibIntendedY = 0;
	int LastLayoutWidth = 0;
	int LastLayoutHeight = 0;
};

class NativeAutoLootSettingsWnd : public CCustomWnd
{
public:
	NativeAutoLootSettingsWnd() : CCustomWnd((char*)"NativeAutoLootSettingsWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAutoLootSettingsWnd);

		SummaryLabel = GetChildItem("AALS_SummaryLabel");
		GroupSummaryLabel = GetChildItem("AALS_GroupSummaryLabel");
		StatusLabel = GetChildItem("AALS_StatusLabel");
		AutoLootCheck = (CButtonWnd*)GetChildItem("AALS_AutoLootCheck");
		NeedGreedCheck = (CButtonWnd*)GetChildItem("AALS_NeedGreedCheck");
		AutoShowCheck = (CButtonWnd*)GetChildItem("AALS_AutoShowCheck");
		ShowNewCheck = (CButtonWnd*)GetChildItem("AALS_ShowNewCheck");
		ConfirmRemoveCheck = (CButtonWnd*)GetChildItem("AALS_ConfirmRemoveCheck");
		AutoRemoveLoreCheck = (CButtonWnd*)GetChildItem("AALS_AutoRemoveLoreCheck");
		RefreshButton = (CButtonWnd*)GetChildItem("AALS_RefreshButton");
		GroupNoneButton = (CButtonWnd*)GetChildItem("AALS_GroupNoneButton");
		GroupSoloButton = (CButtonWnd*)GetChildItem("AALS_GroupSoloButton");
		GroupMasterButton = (CButtonWnd*)GetChildItem("AALS_GroupMasterButton");
		GroupRobinButton = (CButtonWnd*)GetChildItem("AALS_GroupRobinButton");
		GroupKillerButton = (CButtonWnd*)GetChildItem("AALS_GroupKillerButton");

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
			if (pWnd == (CXWnd*)AutoLootCheck) {
				NativeAutoLootSendCommand(NativeAutoLootToggleEnabledCommand());
				SetStatus("Toggled Advanced Loot.");
				return 1;
			}

			if (pWnd == (CXWnd*)NeedGreedCheck) {
				NativeAutoLootSendCommand(NativeAutoLootToggleMasterCandidateCommand());
				SetStatus("Toggled Master Looter candidate.");
				return 1;
			}

			if (pWnd == (CXWnd*)AutoShowCheck) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoShow ? "/say #advloot autoshow off" : "/say #advloot autoshow on");
				SetStatus("Toggled Auto Show.");
				return 1;
			}

			if (pWnd == (CXWnd*)ShowNewCheck) {
				NativeAutoLootSendCommand(gNativeAutoLootShowNewOnly ? "/say #advloot shownew off" : "/say #advloot shownew on");
				SetStatus("Toggled Unfiltered Only.");
				return 1;
			}

			if (pWnd == (CXWnd*)ConfirmRemoveCheck) {
				NativeAutoLootSendCommand(gNativeAutoLootConfirmRemove ? "/say #advloot confirmremove off" : "/say #advloot confirmremove on");
				SetStatus("Toggled Confirm Remove.");
				return 1;
			}

			if (pWnd == (CXWnd*)AutoRemoveLoreCheck) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoRemoveLore ? "/say #advloot autoremovelore off" : "/say #advloot autoremovelore on");
				SetStatus("Toggled Auto Remove Lore.");
				return 1;
			}

			if (pWnd == (CXWnd*)RefreshButton) {
				NativeAutoLootSendCommand("/say #advloot native status");
				SetStatus("Refreshing Advanced Loot settings...");
				return 1;
			}

			if (pWnd == (CXWnd*)GroupNoneButton) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoSplit ? "/say #advloot autosplit off" : "/say #advloot autosplit on");
				SetStatus("Toggled Auto Split Coin.");
				return 1;
			}

			if (pWnd == (CXWnd*)GroupSoloButton) {
				NativeAutoLootSendCommand(gNativeAutoLootAutoLootAll ? "/say #advloot autolootall off" : "/say #advloot autolootall on");
				SetStatus("Toggled Auto Loot All.");
				return 1;
			}

			if (pWnd == (CXWnd*)GroupMasterButton) {
				NativeAutoLootSendCommand(gNativeAutoLootDebug ? "/say #advloot debug off" : "/say #advloot debug on");
				SetStatus("Toggled debug chat.");
				return 1;
			}

			if (pWnd == (CXWnd*)GroupRobinButton) {
				NativeAutoLootSendCommand(gNativeAutoLootLog ? "/say #advloot log off" : "/say #advloot log on");
				SetStatus("Toggled audit log.");
				return 1;
			}

			if (pWnd == (CXWnd*)GroupKillerButton) {
				NativeAutoLootSendCommand("/say #advloot native status");
				SetStatus("Refreshing Advanced Loot settings.");
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
	void SetLabel(CXWnd* label, const char* text);
	void SetButtonCheck(CButtonWnd* button, bool checked);

	CXWnd* SummaryLabel = nullptr;
	CXWnd* GroupSummaryLabel = nullptr;
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* AutoLootCheck = nullptr;
	CButtonWnd* NeedGreedCheck = nullptr;
	CButtonWnd* AutoShowCheck = nullptr;
	CButtonWnd* ShowNewCheck = nullptr;
	CButtonWnd* ConfirmRemoveCheck = nullptr;
	CButtonWnd* AutoRemoveLoreCheck = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* GroupNoneButton = nullptr;
	CButtonWnd* GroupSoloButton = nullptr;
	CButtonWnd* GroupMasterButton = nullptr;
	CButtonWnd* GroupRobinButton = nullptr;
	CButtonWnd* GroupKillerButton = nullptr;
};

static std::vector<NativeAutoLootManagePlayer> gNativeAutoLootManagePlayers;
static int gNativeAutoLootManageEntryId = 0;
static int gNativeAutoLootManageMasterId = 0;
static int gNativeAutoLootManageRollSeconds = 0;
static bool gNativeAutoLootManageCanManage = false;
static bool gNativeAutoLootManageFreeGrab = false;
static bool gNativeAutoLootManageAutoRoll = false;
static std::string gNativeAutoLootManageItemName;
static std::string gNativeAutoLootManageSource;
static std::string gNativeAutoLootManageState;
static std::string gNativeAutoLootManageMasterName;

class NativeAutoLootManageWnd : public CCustomWnd
{
public:
	NativeAutoLootManageWnd() : CCustomWnd((char*)"NativeAutoLootManageWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAutoLootManageWnd);

		SummaryLabel = GetChildItem("AALM_SummaryLabel");
		StatusLabel = GetChildItem("AALM_StatusLabel");
		PlayerList = (CListWnd*)GetChildItem("AALM_PlayerList");
		RefreshButton = (CButtonWnd*)GetChildItem("AALM_RefreshButton");
		AskButton = (CButtonWnd*)GetChildItem("AALM_AskButton");
		RollButton = (CButtonWnd*)GetChildItem("AALM_RollButton");
		FreeGrabButton = (CButtonWnd*)GetChildItem("AALM_FreeGrabButton");
		GiveButton = (CButtonWnd*)GetChildItem("AALM_GiveButton");
		LeaveButton = (CButtonWnd*)GetChildItem("AALM_LeaveButton");
		SetMasterButton = (CButtonWnd*)GetChildItem("AALM_SetMasterButton");
		ClearMasterButton = (CButtonWnd*)GetChildItem("AALM_ClearMasterButton");

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
				RequestRefresh();
				return 1;
			}

			if (gNativeAutoLootManageEntryId <= 0) {
				SetStatus("Select shared loot from the main window.");
				return 1;
			}

			char command[160];
			if (pWnd == (CXWnd*)AskButton) {
				sprintf_s(command, "/say #advloot action %d ask", gNativeAutoLootManageEntryId);
				NativeAutoLootSendCommand(command);
				SetStatus("Started Ask/Roll.");
				return 1;
			}

			if (pWnd == (CXWnd*)RollButton) {
				sprintf_s(command, "/say #advloot action %d roll", gNativeAutoLootManageEntryId);
				NativeAutoLootSendCommand(command);
				SetStatus("Resolved roll.");
				return 1;
			}

			if (pWnd == (CXWnd*)FreeGrabButton) {
				sprintf_s(command, "/say #advloot action %d freegrab", gNativeAutoLootManageEntryId);
				NativeAutoLootSendCommand(command);
				SetStatus("Set Free Grab.");
				return 1;
			}

			if (pWnd == (CXWnd*)LeaveButton) {
				sprintf_s(command, "/say #advloot action %d leave", gNativeAutoLootManageEntryId);
				NativeAutoLootSendCommand(command);
				SetStatus("Left item on corpse.");
				return 1;
			}

			NativeAutoLootManagePlayer* player = GetSelectedPlayer();
			if (!player && (pWnd == (CXWnd*)GiveButton || pWnd == (CXWnd*)SetMasterButton)) {
				SetStatus("Select an eligible player first.");
				return 1;
			}

			if (pWnd == (CXWnd*)GiveButton) {
				sprintf_s(command, "/say #advloot action %d give %s", gNativeAutoLootManageEntryId, player->name.c_str());
				NativeAutoLootSendCommand(command);
				SetStatus("Assigned selected player.");
				return 1;
			}

			if (pWnd == (CXWnd*)SetMasterButton) {
				sprintf_s(command, "/say #advloot master set %s", player->name.c_str());
				NativeAutoLootSendCommand(command);
				SetStatus("Set selected player as Master Looter.");
				return 1;
			}

			if (pWnd == (CXWnd*)ClearMasterButton) {
				NativeAutoLootSendCommand("/say #advloot master clear");
				SetStatus("Cleared Master Looter.");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void RequestRefresh()
	{
		if (gNativeAutoLootManageEntryId <= 0) {
			SetStatus("Select shared loot from the main window.");
			return;
		}

		char command[128];
		sprintf_s(command, "/say #advloot manage %d", gNativeAutoLootManageEntryId);
		NativeAutoLootSendCommand(command);
		SetStatus("Refreshing Manage Loot...");
	}

	void SetStatus(const char* text)
	{
		if (StatusLabel) {
			CXStr value(text ? text : "");
			StatusLabel->SetWindowTextA(value);
		}
	}

	void PulseTick()
	{
		__try {
			if (PlayerList && ((DWORD*)PlayerList)[0x21C / 4] != 30) {
				((DWORD*)PlayerList)[0x21C / 4] = 30;
				((CXWnd*)PlayerList)->Show(0, 1);
				((CXWnd*)PlayerList)->Show(1, 1);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}

		UpdateSummary();
	}

	void UpdateSummary()
	{
		char summary[256];
		if (gNativeAutoLootManageEntryId > 0) {
			sprintf_s(
				summary,
				"%s from %s  Master: %s  State: %s  Roll: %ds",
				gNativeAutoLootManageItemName.empty() ? "Loot" : gNativeAutoLootManageItemName.c_str(),
				gNativeAutoLootManageSource.empty() ? "corpse" : gNativeAutoLootManageSource.c_str(),
				gNativeAutoLootManageMasterName.empty() ? "-" : gNativeAutoLootManageMasterName.c_str(),
				gNativeAutoLootManageState.empty() ? "waiting" : gNativeAutoLootManageState.c_str(),
				gNativeAutoLootManageRollSeconds
			);
		}
		else {
			sprintf_s(summary, "Select a shared loot row to manage.");
		}
		SetLabel(SummaryLabel, summary);
	}

	// Rebuilds the player list. Only event-driven updates may call this:
	// the per-frame pulse uses PulseTick instead, because DeleteAll here
	// would wipe the user's selection before Give/Set ML could read it.
	void RefreshRows()
	{
		UpdateSummary();

		if (!PlayerList) {
			return;
		}

		int selected_character_id = 0;
		const int selected = PlayerList->GetCurSel();
		if (selected >= 0) {
			selected_character_id = (int)PlayerList->GetItemData(selected);
		}

		PlayerList->DeleteAll();
		if (gNativeAutoLootManagePlayers.empty()) {
			CXStr dash("-");
			const int row = PlayerList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
			CXStr empty("No eligible players loaded.");
			PlayerList->SetItemText(row, 1, &empty);
			PlayerList->SetItemText(row, 2, &dash);
			PlayerList->SetItemText(row, 3, &dash);
			return;
		}

		for (const NativeAutoLootManagePlayer& player : gNativeAutoLootManagePlayers) {
			CXStr name(player.name.c_str());
			const int row = PlayerList->AddString(name, player.eligible ? 0xFFFFFFFF : 0xFFB0B0B0, (uint32_t)player.character_id, nullptr, nullptr);
			CXStr vote(player.vote.c_str());
			CXStr master(player.master ? "Master" : "");
			CXStr eligible(player.eligible ? "Yes" : "No");
			PlayerList->SetItemText(row, 1, &vote);
			PlayerList->SetItemText(row, 2, &master);
			PlayerList->SetItemText(row, 3, &eligible);

			if (selected_character_id > 0 && player.character_id == selected_character_id) {
				PlayerList->SetCurSel(row);
			}
		}
	}

private:
	NativeAutoLootManagePlayer* GetSelectedPlayer()
	{
		if (!PlayerList) {
			return nullptr;
		}

		const int selected = PlayerList->GetCurSel();
		if (selected < 0) {
			return nullptr;
		}

		const int character_id = (int)PlayerList->GetItemData(selected);
		if (character_id <= 0) {
			return nullptr;
		}

		for (NativeAutoLootManagePlayer& player : gNativeAutoLootManagePlayers) {
			if (player.character_id == character_id) {
				return &player;
			}
		}

		return nullptr;
	}

	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	CXWnd* SummaryLabel = nullptr;
	CXWnd* StatusLabel = nullptr;
	CListWnd* PlayerList = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* AskButton = nullptr;
	CButtonWnd* RollButton = nullptr;
	CButtonWnd* FreeGrabButton = nullptr;
	CButtonWnd* GiveButton = nullptr;
	CButtonWnd* LeaveButton = nullptr;
	CButtonWnd* SetMasterButton = nullptr;
	CButtonWnd* ClearMasterButton = nullptr;
};

static NativeAutoLootRulesWnd* gNativeAutoLootRulesWnd = nullptr;
static NativeAutoLootSettingsWnd* gNativeAutoLootSettingsWnd = nullptr;
static NativeAutoLootManageWnd* gNativeAutoLootManageWnd = nullptr;

enum NativeAutoLootMenuAction
{
	kAALMenuInspect = 1,
	kAALMenuLoot,
	kAALMenuLeave,
	kAALMenuNever,
	kAALMenuFreeGrab,
	kAALMenuGive,
	kAALMenuLink,
	kAALMenuSelectCorpse
};

static int gNativeAutoLootMenuEntryId = 0;
static bool gNativeAutoLootMenuShared = false;

// Live-style right-click menu, implemented as a small popup window built
// from the same proven primitives as the loot windows (real engine context
// menus would require unverified CContextMenu offsets in this client).
class NativeAutoLootMenuWnd : public CCustomWnd
{
public:
	NativeAutoLootMenuWnd() : CCustomWnd((char*)"NativeAutoLootMenuWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeAutoLootMenuWnd);
		ActionList = (CListWnd*)GetChildItem("AALM_List");
		pXWnd()->Show(0, 1);
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK && pWnd == (CXWnd*)ActionList && ActionList) {
			const int selected = ActionList->GetCurSel();
			const int action = selected >= 0 ? (int)ActionList->GetItemData(selected) : 0;
			pXWnd()->Show(0, 1);
			ExecuteAction(action);
			return 1;
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void PopupForRow(bool shared, int entry_id, int anchor_x, int anchor_y)
	{
		if (!ActionList) {
			return;
		}

		gNativeAutoLootMenuShared = shared;
		gNativeAutoLootMenuEntryId = entry_id;

		__try {
			if (((DWORD*)ActionList)[0x21C / 4] != 24) {
				((DWORD*)ActionList)[0x21C / 4] = 24;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("menu row height apply faulted");
		}

		ActionList->DeleteAll();
		AddEntry("Inspect Item", kAALMenuInspect);
		if (shared) {
			AddEntry("Free Grab", kAALMenuFreeGrab);
			AddEntry("Leave on Corpse", kAALMenuLeave);
			AddEntry("Give To...", kAALMenuGive);
		}
		else {
			AddEntry("Loot", kAALMenuLoot);
			AddEntry("Leave", kAALMenuLeave);
			AddEntry("Never", kAALMenuNever);
		}
		AddEntry("Link Corpse Loot", kAALMenuLink);
		AddEntry("Select Corpse", kAALMenuSelectCorpse);

		PCSIDLWND raw = (PCSIDLWND)this;
		const int width = 220;
		const int height = 156;
		raw->Location.left = anchor_x;
		raw->Location.top = anchor_y;
		raw->Location.right = anchor_x + width;
		raw->Location.bottom = anchor_y + height;
		pXWnd()->Show(0, 1);
		pXWnd()->Show(1, 1);
	}

private:
	void AddEntry(const char* label, int action)
	{
		CXStr text(label);
		ActionList->AddString(text, 0xFFFFFFFF, (uint32_t)action, nullptr, label);
	}

	void ExecuteAction(int action)
	{
		if (action <= 0 || gNativeAutoLootMenuEntryId <= 0) {
			return;
		}

		const int entry_id = gNativeAutoLootMenuEntryId;
		char command[160];
		switch (action) {
		case kAALMenuInspect:
			sprintf_s(command, "/say #advloot inspect %d", entry_id);
			break;
		case kAALMenuLoot:
			sprintf_s(command, "/say #advloot action %d loot", entry_id);
			break;
		case kAALMenuLeave:
			sprintf_s(command, "/say #advloot action %d leave", entry_id);
			break;
		case kAALMenuNever:
			sprintf_s(command, "/say #advloot action %d never", entry_id);
			break;
		case kAALMenuFreeGrab:
			sprintf_s(command, "/say #advloot action %d freegrab", entry_id);
			break;
		case kAALMenuGive:
			NativeAutoLootShowManageWindow(entry_id);
			return;
		case kAALMenuLink:
			sprintf_s(command, "/say #advloot corpse link %d", entry_id);
			break;
		case kAALMenuSelectCorpse:
			sprintf_s(command, "/say #advloot corpse target %d", entry_id);
			break;
		default:
			return;
		}

		NativeAutoLootSendCommand(command);
	}

	CListWnd* ActionList = nullptr;
};

static NativeAutoLootMenuWnd* gNativeAutoLootMenuWnd = nullptr;

static void NativeAutoLootShowRowMenu(bool shared, int entry_id, int anchor_x, int anchor_y)
{
	if (!gNativeAutoLootMenuWnd) {
		NativeAutoLootTrace("creating menu window");
		gNativeAutoLootMenuWnd = new NativeAutoLootMenuWnd();
	}

	gNativeAutoLootMenuWnd->PopupForRow(shared, entry_id, anchor_x, anchor_y);
}

static void NativeAutoLootHideRowMenu()
{
	if (gNativeAutoLootMenuWnd) {
		gNativeAutoLootMenuWnd->pXWnd()->Show(0, 1);
	}
}
static std::vector<NativeAutoLootRuleRow> gNativeAutoLootRuleRows;

static const char* NativeAutoLootToggleEnabledCommand()
{
	return gNativeAutoLootEnabled ? "/say #advloot off" : "/say #advloot on";
}

static const char* NativeAutoLootToggleApplyFiltersCommand()
{
	return gNativeAutoLootApplyFilters ? "/say #advloot applyfilters off" : "/say #advloot applyfilters on";
}

static const char* NativeAutoLootToggleMasterCandidateCommand()
{
	return gNativeAutoLootMasterCandidate ? "/say #advloot masterlooter off" : "/say #advloot masterlooter on";
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

static long long NativeToInt64(const std::string& value, long long fallback = 0)
{
	if (value.empty()) {
		return fallback;
	}

	return _strtoi64(value.c_str(), nullptr, 10);
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

static std::string NativeLower(std::string value)
{
	for (char& ch : value) {
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}

	return value;
}

static bool NativeContainsLower(const std::string& value, const std::string& search_lower)
{
	return search_lower.empty() || NativeLower(value).find(search_lower) != std::string::npos;
}

struct NativeItemPowerInfo
{
	int item_id = 0;
	int item_level = 0;
	int score = 0;
	int version = 0;
	std::string role;
	std::string source;
	std::string name;
	DWORD last_seen = 0;
};

struct NativeItemRarityInfo
{
	int item_id = 0;
	int rarity = 0;
	std::string name;
	DWORD last_seen = 0;
};

static std::unordered_map<int, NativeItemPowerInfo> gNativeItemPowerById;
static std::unordered_map<int, NativeItemRarityInfo> gNativeItemRarityById;

static bool NativeItemPowerParseTransport(const char* message)
{
	if (!message || !NativeStartsWith(message, "ITEMPOWER|")) {
		return false;
	}

	const std::string payload(message);
	const int item_id = NativeToInt(NativeGetPairValue(payload, "item_id"));
	if (item_id <= 0) {
		return true;
	}

	if (NativeStartsWith(message, "ITEMPOWER|clear|")) {
		gNativeItemPowerById.erase(item_id);
		NativeAutoLootTrace("ItemPower cleared item_id=%d", item_id);
		return true;
	}

	NativeItemPowerInfo info;
	info.item_id = item_id;
	info.item_level = NativeToInt(NativeGetPairValue(payload, "ilvl"));
	info.score = NativeToInt(NativeGetPairValue(payload, "score"));
	info.version = NativeToInt(NativeGetPairValue(payload, "version"));
	info.role = NativeGetPairValue(payload, "role");
	info.source = NativeGetPairValue(payload, "source");
	info.name = NativeGetPairValue(payload, "name");
	info.last_seen = GetTickCount();

	if (info.item_level > 0 || info.score > 0) {
		gNativeItemPowerById[item_id] = info;
		NativeAutoLootTrace(
			"ItemPower cached item_id=%d ilvl=%d score=%d role=%s source=%s",
			info.item_id,
			info.item_level,
			info.score,
			info.role.c_str(),
			info.source.c_str()
		);
	}

	return true;
}

static bool NativeItemRarityParseTransport(const char* message)
{
	if (!message || !NativeStartsWith(message, "ITEMRARITY|")) {
		return false;
	}

	const std::string payload(message);
	const int item_id = NativeToInt(NativeGetPairValue(payload, "item_id"));
	if (item_id <= 0) {
		return true;
	}

	if (NativeStartsWith(message, "ITEMRARITY|clear|")) {
		gNativeItemRarityById.erase(item_id);
		NativeAutoLootTrace("ItemRarity cleared item_id=%d", item_id);
		return true;
	}

	NativeItemRarityInfo info;
	info.item_id = item_id;
	info.rarity = NativeToInt(NativeGetPairValue(payload, "rarity"));
	info.name = NativeGetPairValue(payload, "name");
	info.last_seen = GetTickCount();
	if (info.rarity >= 0 && info.rarity <= 4) {
		gNativeItemRarityById[item_id] = info;
		NativeAutoLootTrace("ItemRarity cached item_id=%d rarity=%d name=%s", item_id, info.rarity, info.name.c_str());
	}

	return true;
}

template <size_t Size>
static void NativeCopyText(char (&destination)[Size], const std::string& source)
{
	memset(destination, 0, Size);
	strncpy_s(destination, source.c_str(), Size - 1);
}

struct NativeMulticlassClassDef
{
	int id = 0;
	const char* name = "";
	const char* abbreviation = "";
};

static const NativeMulticlassClassDef gNativeMulticlassClassDefs[] = {
	{1, "Warrior", "WAR"},
	{2, "Cleric", "CLR"},
	{3, "Paladin", "PAL"},
	{4, "Ranger", "RNG"},
	{5, "Shadow Knight", "SHD"},
	{6, "Druid", "DRU"},
	{7, "Monk", "MNK"},
	{8, "Bard", "BRD"},
	{9, "Rogue", "ROG"},
	{10, "Shaman", "SHM"},
	{11, "Necromancer", "NEC"},
	{12, "Wizard", "WIZ"},
	{13, "Magician", "MAG"},
	{14, "Enchanter", "ENC"},
	{15, "Beastlord", "BST"},
	{16, "Berserker", "BER"}
};

struct NativeMulticlassPetRow
{
	int id = 0;
	int hp = 0;
	int mana = 0;
	bool taunt = false;
	bool hold = false;
	bool spellhold = false;
	bool focused = false;
	std::string name = "Pet";
	std::string order = "follow";
	std::string target = "-";
};

struct NativeMulticlassMelodySlot
{
	int slot = 0;
	int spell_id = 0;
	int level = 0;
	std::string name = "-";
	std::string state = "empty";
};

struct NativeMulticlassMelodySong
{
	int spell_id = 0;
	int level = 0;
	bool allowed = false;
	std::string name = "Song";
	std::string reason;
};

struct NativeMulticlassDisciplineRow
{
	int slot = 0;
	int spell_id = 0;
	int level = 0;
	int timer = 0;
	int timer_total = 0;
	DWORD timer_received_ms = 0;
	bool ready = false;
	std::string name = "Discipline";
	std::string state = "Blocked";
};

struct NativeMulticlassState
{
	bool has_profile = false;
	bool locked = false;
	bool can_choose = false;
	bool multiple_pets = false;
	int class1 = 0;
	int class2 = 0;
	int class3 = 0;
	int presentation = 0;
	int base = 0;
	int reweaves = 0;
	int selected_slot2 = 0;
	int selected_slot3 = 0;
	int roster_count = 0;
	int roster_limit = 1;
	int focus_id = 0;
	int mana = 0;
	int max_mana = 0;
	int endurance = 0;
	int max_endurance = 0;
	int class_mask = 0;
	int aa_mask = 0;
	std::string profile_name = "Unchosen Trio";
	std::string resonance_key;
	std::string class1_name = "Base";
	std::string class2_name = "Unchosen";
	std::string class3_name = "Unchosen";
	std::string presentation_name = "Unknown";
	std::string base_name = "Unknown";
	std::string roles = "-";
	std::string resonance = "Trio Notes";
	std::string summary = "Waiting for Multiclass profile.";
	std::string skill_summary = "Skills: waiting for profile.";
	std::string bonus_summary = "Resonance bonuses: waiting for profile.";
	std::string selection_status = "Waiting for Multiclass profile...";
	std::string pet_policy = "single-pet";
	std::string pet_control = "Pet console pending.";
	bool has_bard = false;
	std::string melody_status = "Bard Melody: waiting for profile.";
	std::string discipline_status = "Disciplines: waiting for profile.";
};

class NativeMulticlassWnd;
class NativeMulticlassPetWnd;
class NativeMulticlassMelodyWnd;
class NativeMulticlassDisciplineWnd;

static NativeMulticlassWnd* gNativeMulticlassWnd = nullptr;
static NativeMulticlassPetWnd* gNativeMulticlassPetWnd = nullptr;
static NativeMulticlassMelodyWnd* gNativeMulticlassMelodyWnd = nullptr;
static NativeMulticlassDisciplineWnd* gNativeMulticlassDisciplineWnd = nullptr;
static NativeMulticlassState gNativeMulticlassState;
static std::vector<NativeMulticlassPetRow> gNativeMulticlassPets;
static std::vector<NativeMulticlassMelodySlot> gNativeMulticlassMelodySlots;
static std::vector<NativeMulticlassMelodySong> gNativeMulticlassMelodySongs;
static std::vector<NativeMulticlassDisciplineRow> gNativeMulticlassDisciplineRows;
static bool gNativeMulticlassSentStatus = false;
static bool gNativeMulticlassContextMenuHookInstalled = false;
static bool gNativeMulticlassContextMenuHookEnabled = true;
static bool gNativeMulticlassSpellLevelsLoading = false;
static int gNativeMulticlassSpellLevelPatchCount = 0;
static int gNativeMulticlassSpellLevelReapplyDelay = 0;
static int gNativeMulticlassSpellLevelReapplyPasses = 0;
static int gNativeMulticlassShowCasterUiPulses = 0;
static std::unordered_map<int, int> gNativeMulticlassSpellLevelsById;
static std::unordered_map<std::string, int> gNativeMulticlassSpellLevelsByName;

static void NativeMulticlassRefreshWindows();
static void NativeMulticlassNormalizeSelections();
static void NativeMulticlassScheduleSpellLevelReapply();
static void NativeMulticlassMaintainSpellLevelPatches();
static bool NativeMulticlassRewriteSpellMenuText(const char* text, unsigned int menu_id, std::string& rewritten);

static bool NativeMulticlassIsPlayerClass(int class_id)
{
	return class_id >= 1 && class_id <= 16;
}

static const char* NativeMulticlassClassName(int class_id)
{
	for (const auto& class_def : gNativeMulticlassClassDefs) {
		if (class_def.id == class_id) {
			return class_def.name;
		}
	}

	return "Unchosen";
}

static const char* NativeMulticlassClassAbbreviation(int class_id)
{
	for (const auto& class_def : gNativeMulticlassClassDefs) {
		if (class_def.id == class_id) {
			return class_def.abbreviation;
		}
	}

	return "";
}

static DWORD NativeMulticlassToDword(int value)
{
	return value > 0 ? (DWORD)value : 0;
}

static bool NativeMulticlassPresentationUsesMana(int class_id)
{
	switch (class_id) {
		case 2:  // Cleric
		case 3:  // Paladin
		case 4:  // Ranger
		case 5:  // Shadow Knight
		case 6:  // Druid
		case 8:  // Bard
		case 10: // Shaman
		case 11: // Necromancer
		case 12: // Wizard
		case 13: // Magician
		case 14: // Enchanter
		case 15: // Beastlord
			return true;
		default:
			return false;
	}
}

static void NativeMulticlassScheduleCasterUI()
{
	if (
		gNativeMulticlassState.has_profile &&
		NativeMulticlassPresentationUsesMana(gNativeMulticlassState.presentation)
	) {
		gNativeMulticlassShowCasterUiPulses = 240;
	}
}

static void NativeMulticlassPatchPlayerData(EQPlayer* player)
{
	if (!player) {
		return;
	}

	if (NativeMulticlassIsPlayerClass(gNativeMulticlassState.presentation)) {
		player->Data.Class = (BYTE)gNativeMulticlassState.presentation;
	}

	if (gNativeMulticlassState.max_mana > 0) {
		player->Data.ManaCurrent = NativeMulticlassToDword(gNativeMulticlassState.mana);
		player->Data.ManaMax = NativeMulticlassToDword(gNativeMulticlassState.max_mana);
	}

	if (gNativeMulticlassState.max_endurance > 0) {
		player->Data.EnduranceMax = NativeMulticlassToDword(gNativeMulticlassState.max_endurance);
	}
}

static void NativeMulticlassPatchLocalVitals()
{
	if (
		!gNativeMulticlassState.has_profile ||
		!NativeMulticlassPresentationUsesMana(gNativeMulticlassState.presentation) ||
		!ppCharData ||
		!pCharData
	) {
		return;
	}

	__try {
		PCHARINFO2 char_info = GetCharInfo2();
		if (char_info) {
			if (NativeMulticlassIsPlayerClass(gNativeMulticlassState.presentation)) {
				char_info->Class = (DWORD)gNativeMulticlassState.presentation;
			}
			if (gNativeMulticlassState.max_mana > 0) {
				char_info->Mana = NativeMulticlassToDword(gNativeMulticlassState.mana);
			}
			if (gNativeMulticlassState.max_endurance > 0) {
				char_info->Endurance = NativeMulticlassToDword(gNativeMulticlassState.endurance);
			}
		}

		if (ppLocalPlayer) {
			NativeMulticlassPatchPlayerData(pLocalPlayer);
		}
		if (ppCharSpawn && pCharSpawn && (!ppLocalPlayer || pCharSpawn != pLocalPlayer)) {
			NativeMulticlassPatchPlayerData(pCharSpawn);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
}

static void NativeMulticlassShowPlayerManaPiece(const char* child_name)
{
	if (!child_name || !pPlayerWnd) {
		return;
	}

	CXWnd* child = nullptr;
	__try {
		child = pPlayerWnd->GetChildItem((char*)child_name);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		child = nullptr;
	}

	if (child) {
		child->Show(1, 1);
	}
}

static void NativeMulticlassShowSpellGemWindow()
{
	if (!ppCastSpellWnd || !pCastSpellWnd) {
		return;
	}

	__try {
		CXWnd* spell_window = (CXWnd*)pCastSpellWnd;
		if (spell_window) {
			spell_window->Show(1, 1);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
}

static int NativeMulticlassGetLocalClass()
{
	__try {
		PCHARINFO2 char_info = GetCharInfo2();
		if (char_info && NativeMulticlassIsPlayerClass(static_cast<int>(char_info->Class))) {
			return static_cast<int>(char_info->Class);
		}

		if (ppLocalPlayer && pLocalPlayer && NativeMulticlassIsPlayerClass(static_cast<int>(pLocalPlayer->Data.Class))) {
			return static_cast<int>(pLocalPlayer->Data.Class);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}

	return 0;
}

static bool NativeMulticlassIsUsableClassName(const std::string& class_name)
{
	return !class_name.empty() &&
		class_name != "Base" &&
		class_name != "Unchosen" &&
		class_name != "Unknown";
}

static void NativeMulticlassAppendInventoryClassRow(
	std::string& text,
	int class_id,
	const std::string& class_name
)
{
	if (!NativeMulticlassIsPlayerClass(class_id)) {
		return;
	}

	const std::string row_name = NativeMulticlassIsUsableClassName(class_name) ?
		class_name :
		NativeMulticlassClassName(class_id);

	const char* abbreviation = NativeMulticlassClassAbbreviation(class_id);
	if (!abbreviation || !abbreviation[0] || row_name.empty() || row_name == "Unchosen") {
		return;
	}

	if (!text.empty()) {
		text += "\n";
	}

	text += abbreviation;
	text += " ";
	text += row_name;
}

static std::string NativeMulticlassInventoryClassText()
{
	std::string text;

	if (gNativeMulticlassState.has_profile) {
		NativeMulticlassAppendInventoryClassRow(
			text,
			gNativeMulticlassState.class1,
			gNativeMulticlassState.class1_name
		);
		NativeMulticlassAppendInventoryClassRow(
			text,
			gNativeMulticlassState.class2,
			gNativeMulticlassState.class2_name
		);
		NativeMulticlassAppendInventoryClassRow(
			text,
			gNativeMulticlassState.class3,
			gNativeMulticlassState.class3_name
		);
	}

	if (text.empty()) {
		const int local_class = NativeMulticlassGetLocalClass();
		NativeMulticlassAppendInventoryClassRow(text, local_class, "");
	}

	return text;
}

static void NativeMulticlassPatchInventoryClassLabel()
{
	if (!ppInventoryWnd || !pInventoryWnd) {
		return;
	}

	const std::string class_text = NativeMulticlassInventoryClassText();
	if (class_text.empty()) {
		return;
	}

	__try {
		CXWnd* class_label = pInventoryWnd->GetChildItem((char*)"IW_Class");
		if (class_label) {
			CXStr value(class_text.c_str());
			class_label->SetWindowTextA(value);
			class_label->Show(1, 1);
		}

		CXWnd* abbreviation_label = pInventoryWnd->GetChildItem((char*)"IW_ClassAbbr");
		if (abbreviation_label) {
			abbreviation_label->Show(1, 1);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
}

static void NativeMulticlassMaintainPresentationUI()
{
	NativeMulticlassPatchInventoryClassLabel();

	if (!gNativeMulticlassState.has_profile) {
		return;
	}

	if (NativeMulticlassPresentationUsesMana(gNativeMulticlassState.presentation) && pPlayerWnd) {
		NativeMulticlassPatchLocalVitals();
		NativeMulticlassShowPlayerManaPiece("Player_Mana");
		NativeMulticlassShowPlayerManaPiece("PlayerMana");
		NativeMulticlassShowPlayerManaPiece("Player_ManaLabel");
		NativeMulticlassShowPlayerManaPiece("ManaLabel");
		NativeMulticlassShowPlayerManaPiece("Player_ManaPercLabel");
		NativeMulticlassShowPlayerManaPiece("ManPercLabel");

		if (gNativeMulticlassShowCasterUiPulses > 0) {
			--gNativeMulticlassShowCasterUiPulses;
			NativeMulticlassShowSpellGemWindow();
		}
	}
}

static std::string NativeMulticlassNormalizedName(const std::string& value)
{
	std::string normalized;
	normalized.reserve(value.size());
	bool previous_space = false;
	for (char c : value) {
		const unsigned char ch = static_cast<unsigned char>(c);
		if (isspace(ch)) {
			if (!normalized.empty() && !previous_space) {
				normalized.push_back(' ');
			}
			previous_space = true;
			continue;
		}

		normalized.push_back(static_cast<char>(tolower(ch)));
		previous_space = false;
	}

	while (!normalized.empty() && normalized[normalized.size() - 1] == ' ') {
		normalized.resize(normalized.size() - 1);
	}

	return normalized;
}

static void NativeMulticlassSetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text ? text : "");
		label->SetWindowTextA(value);
	}
}

static void NativeMulticlassSetButtonText(CButtonWnd* button, const char* text)
{
	if (button) {
		CXStr value(text ? text : "");
		((CXWnd*)button)->SetWindowTextA(value);
	}
}

static void NativeMulticlassSetVisible(CXWnd* wnd, bool visible)
{
	if (wnd) {
		wnd->Show(visible ? 1 : 0, 1);
	}
}

static bool NativeMulticlassClassAvailableForSlot(int class_id, int slot)
{
	if (!NativeMulticlassIsPlayerClass(class_id)) {
		return false;
	}

	if (class_id == gNativeMulticlassState.class1) {
		return false;
	}

	if (slot == 2 && class_id == gNativeMulticlassState.selected_slot3) {
		return false;
	}

	if (slot == 3 && class_id == gNativeMulticlassState.selected_slot2) {
		return false;
	}

	return true;
}

static int NativeMulticlassNextClass(int current, int direction, int slot)
{
	if (direction == 0) {
		direction = 1;
	}

	int candidate = NativeMulticlassIsPlayerClass(current) ? current : 1;
	for (int step = 0; step < 16; ++step) {
		candidate += direction > 0 ? 1 : -1;
		if (candidate > 16) {
			candidate = 1;
		}
		else if (candidate < 1) {
			candidate = 16;
		}

		if (NativeMulticlassClassAvailableForSlot(candidate, slot)) {
			return candidate;
		}
	}

	return current;
}

static void NativeMulticlassNormalizeSelections()
{
	if (gNativeMulticlassState.locked) {
		gNativeMulticlassState.selected_slot2 = gNativeMulticlassState.class2;
		gNativeMulticlassState.selected_slot3 = gNativeMulticlassState.class3;
		return;
	}

	if (!NativeMulticlassClassAvailableForSlot(gNativeMulticlassState.selected_slot2, 2)) {
		gNativeMulticlassState.selected_slot2 = NativeMulticlassNextClass(gNativeMulticlassState.class1, 1, 2);
	}

	if (!NativeMulticlassClassAvailableForSlot(gNativeMulticlassState.selected_slot3, 3)) {
		gNativeMulticlassState.selected_slot3 = NativeMulticlassNextClass(gNativeMulticlassState.selected_slot2, 1, 3);
	}
}

static NativeMulticlassPetRow* NativeMulticlassFindPet(int pet_id)
{
	if (pet_id <= 0) {
		return nullptr;
	}

	for (auto& pet : gNativeMulticlassPets) {
		if (pet.id == pet_id) {
			return &pet;
		}
	}

	return nullptr;
}

static bool NativeMulticlassApplyCachedSpellLevel(int spell_id, int level)
{
	if (!pSpellMgr || spell_id <= 0 || spell_id >= TOTAL_SPELL_COUNT || level <= 0 || level > 254) {
		return false;
	}

	PSPELLMGR spell_mgr = (PSPELLMGR)pSpellMgr;
	PSPELL spell = spell_mgr->Spells[spell_id];
	if (!spell) {
		return false;
	}

	auto apply_spell_level = [&](int class_id) {
		if (NativeMulticlassIsPlayerClass(class_id)) {
			spell->Level[class_id - 1] = static_cast<BYTE>(level);
		}
	};

	apply_spell_level(gNativeMulticlassState.presentation);
	apply_spell_level(gNativeMulticlassState.base);
	apply_spell_level(gNativeMulticlassState.class1);
	apply_spell_level(gNativeMulticlassState.class2);
	apply_spell_level(gNativeMulticlassState.class3);

	if (spell->Name[0]) {
		gNativeMulticlassSpellLevelsByName[NativeMulticlassNormalizedName(spell->Name)] = level;
	}

	return true;
}

static int NativeMulticlassApplyCachedSpellLevels()
{
	if (!pSpellMgr || gNativeMulticlassSpellLevelsById.empty()) {
		return 0;
	}

	int applied = 0;
	for (const auto& entry : gNativeMulticlassSpellLevelsById) {
		if (NativeMulticlassApplyCachedSpellLevel(entry.first, entry.second)) {
			++applied;
		}
	}

	return applied;
}

static void NativeMulticlassScheduleSpellLevelReapply()
{
	if (gNativeMulticlassSpellLevelsById.empty()) {
		gNativeMulticlassSpellLevelReapplyDelay = 0;
		gNativeMulticlassSpellLevelReapplyPasses = 0;
		return;
	}

	gNativeMulticlassSpellLevelReapplyDelay = 0;
	gNativeMulticlassSpellLevelReapplyPasses = 24;
}

static void NativeMulticlassMaintainSpellLevelPatches()
{
	if (gNativeMulticlassSpellLevelsLoading || gNativeMulticlassSpellLevelReapplyPasses <= 0) {
		return;
	}

	if (gNativeMulticlassSpellLevelReapplyDelay > 0) {
		--gNativeMulticlassSpellLevelReapplyDelay;
		return;
	}

	const int applied = NativeMulticlassApplyCachedSpellLevels();
	if (
		applied > 0 &&
		applied == static_cast<int>(gNativeMulticlassSpellLevelsById.size()) &&
		gNativeMulticlassSpellLevelReapplyPasses > 6
	) {
		gNativeMulticlassSpellLevelReapplyPasses = (std::min)(gNativeMulticlassSpellLevelReapplyPasses, 6);
	}
	else {
		--gNativeMulticlassSpellLevelReapplyPasses;
	}

	gNativeMulticlassSpellLevelReapplyDelay = 30;
}

static int NativeMulticlassFindCachedSpellLevelByName(const std::string& spell_name)
{
	const auto found_by_name = gNativeMulticlassSpellLevelsByName.find(spell_name);
	if (found_by_name != gNativeMulticlassSpellLevelsByName.end()) {
		return found_by_name->second;
	}

	if (!pSpellMgr || gNativeMulticlassSpellLevelsById.empty()) {
		return 0;
	}

	PSPELLMGR spell_mgr = (PSPELLMGR)pSpellMgr;
	for (const auto& entry : gNativeMulticlassSpellLevelsById) {
		if (entry.first <= 0 || entry.first >= TOTAL_SPELL_COUNT) {
			continue;
		}

		PSPELL spell = spell_mgr->Spells[entry.first];
		if (!spell || !spell->Name[0]) {
			continue;
		}

		const std::string cached_name = NativeMulticlassNormalizedName(spell->Name);
		gNativeMulticlassSpellLevelsByName[cached_name] = entry.second;
		if (cached_name == spell_name) {
			return entry.second;
		}
	}

	return 0;
}

static bool NativeMulticlassApplySpellLevelPatch(const std::string& payload)
{
	const int spell_id = NativeToInt(NativeGetPairValue(payload, "id"));
	const int level = NativeToInt(NativeGetPairValue(payload, "level"), 255);
	const int presentation = NativeToInt(NativeGetPairValue(payload, "presentation"));

	if (spell_id <= 0 || spell_id >= TOTAL_SPELL_COUNT || level <= 0 || level > 254 || presentation < 1 || presentation > 16) {
		NativeAutoLootTrace("multiclass rejected spell level patch: id=%d level=%d presentation=%d", spell_id, level, presentation);
		return true;
	}

	gNativeMulticlassSpellLevelsById[spell_id] = level;
	NativeMulticlassApplyCachedSpellLevel(spell_id, level);
	++gNativeMulticlassSpellLevelPatchCount;
	return true;
}

static bool NativeMulticlassRewriteSpellMenuText(const char* text, unsigned int menu_id, std::string& rewritten)
{
	if (!text || !text[0] || (gNativeMulticlassSpellLevelsByName.empty() && gNativeMulticlassSpellLevelsById.empty())) {
		return false;
	}

	const char* cursor = text;
	while (*cursor == ' ' || *cursor == '\t') {
		++cursor;
	}

	if (!isdigit(static_cast<unsigned char>(*cursor))) {
		return false;
	}

	int displayed_level = 0;
	while (isdigit(static_cast<unsigned char>(*cursor))) {
		displayed_level = (displayed_level * 10) + (*cursor - '0');
		++cursor;
	}

	if (cursor[0] != ' ' || cursor[1] != '-' || cursor[2] != ' ') {
		return false;
	}

	cursor += 3;
	int approved_level = 0;
	const auto found_by_id = gNativeMulticlassSpellLevelsById.find(static_cast<int>(menu_id));
	if (found_by_id != gNativeMulticlassSpellLevelsById.end()) {
		approved_level = found_by_id->second;
	}

	if (!approved_level) {
		const std::string spell_name = NativeMulticlassNormalizedName(cursor);
		approved_level = NativeMulticlassFindCachedSpellLevelByName(spell_name);
	}

	if (approved_level <= 0 || approved_level == displayed_level) {
		return false;
	}

	char buffer[256];
	sprintf_s(buffer, "%d - %s", approved_level, cursor);
	rewritten = buffer;
	return true;
}

class NativeMulticlassWnd : public CCustomWnd
{
public:
	NativeMulticlassWnd() : CCustomWnd((char*)"NativeMulticlassWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeMulticlassWnd);

		StatusLabel = GetChildItem("MCW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("MCW_RefreshButton");
		TrioLabel = GetChildItem("MCW_TrioLabel");
		PresentationLabel = GetChildItem("MCW_PresentationLabel");
		Slot2Label = GetChildItem("MCW_Slot2Label");
		Slot2PrevButton = (CButtonWnd*)GetChildItem("MCW_Slot2PrevButton");
		Slot2ValueLabel = GetChildItem("MCW_Slot2ValueLabel");
		Slot2NextButton = (CButtonWnd*)GetChildItem("MCW_Slot2NextButton");
		Slot3Label = GetChildItem("MCW_Slot3Label");
		Slot3PrevButton = (CButtonWnd*)GetChildItem("MCW_Slot3PrevButton");
		Slot3ValueLabel = GetChildItem("MCW_Slot3ValueLabel");
		Slot3NextButton = (CButtonWnd*)GetChildItem("MCW_Slot3NextButton");
		MelodyButton = (CButtonWnd*)GetChildItem("MCW_MelodyButton");
		DiscsButton = (CButtonWnd*)GetChildItem("MCW_DiscsButton");
		PetsButton = (CButtonWnd*)GetChildItem("MCW_PetsButton");
		LockButton = (CButtonWnd*)GetChildItem("MCW_LockButton");
		InfoHeaderLabel = GetChildItem("MCW_InfoHeaderLabel");
		InfoLabel = GetChildItem("MCW_InfoLabel");

		Layout();
		Refresh();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)RefreshButton) {
				NativeAutoLootSendCommand("/say #mc refresh");
				SetStatus("Refreshing Multiclass profile...");
				return 1;
			}

			if (pWnd == (CXWnd*)PetsButton) {
				NativeMulticlassEnsurePetWindow(true);
				NativeAutoLootSendCommand("/say #mc pets");
				SetStatus("Opening pet console...");
				return 1;
			}

			if (pWnd == (CXWnd*)MelodyButton) {
				NativeMulticlassEnsureMelodyWindow(true);
				NativeAutoLootSendCommand("/say #mc melody open");
				SetStatus("Opening Bard Melody...");
				return 1;
			}

			if (pWnd == (CXWnd*)DiscsButton) {
				NativeMulticlassEnsureDisciplineWindow(true);
				NativeAutoLootSendCommand("/say #mc disc open");
				SetStatus("Opening discipline tools...");
				return 1;
			}

			if (pWnd == (CXWnd*)Slot2PrevButton || pWnd == (CXWnd*)Slot2NextButton) {
				if (!gNativeMulticlassState.locked && gNativeMulticlassState.can_choose) {
					gNativeMulticlassState.selected_slot2 = NativeMulticlassNextClass(gNativeMulticlassState.selected_slot2, pWnd == (CXWnd*)Slot2NextButton ? 1 : -1, 2);
					NativeMulticlassNormalizeSelections();
					Refresh();
				}
				return 1;
			}

			if (pWnd == (CXWnd*)Slot3PrevButton || pWnd == (CXWnd*)Slot3NextButton) {
				if (!gNativeMulticlassState.locked && gNativeMulticlassState.can_choose) {
					gNativeMulticlassState.selected_slot3 = NativeMulticlassNextClass(gNativeMulticlassState.selected_slot3, pWnd == (CXWnd*)Slot3NextButton ? 1 : -1, 3);
					NativeMulticlassNormalizeSelections();
					Refresh();
				}
				return 1;
			}

			if (pWnd == (CXWnd*)LockButton) {
				if (gNativeMulticlassState.locked) {
					SetStatus("This trio is already locked.");
					return 1;
				}

				NativeMulticlassNormalizeSelections();
				if (!NativeMulticlassIsPlayerClass(gNativeMulticlassState.selected_slot2) || !NativeMulticlassIsPlayerClass(gNativeMulticlassState.selected_slot3)) {
					SetStatus("Choose two added classes before locking.");
					return 1;
				}

				char command[128];
				sprintf_s(command, "/say #mc choose %d %d", gNativeMulticlassState.selected_slot2, gNativeMulticlassState.selected_slot3);
				NativeAutoLootSendCommand(command);
				SetStatus("Locking trio...");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
	}

	void SetStatus(const char* text)
	{
		NativeMulticlassSetLabel(StatusLabel, text);
	}

	void Refresh()
	{
		NativeMulticlassNormalizeSelections();

		NativeMulticlassSetLabel(StatusLabel, gNativeMulticlassState.selection_status.c_str());

		char trio[192];
		sprintf_s(
			trio,
			"%s / %s / %s [%s]",
			gNativeMulticlassState.class1_name.c_str(),
			gNativeMulticlassState.class2_name.c_str(),
			gNativeMulticlassState.class3_name.c_str(),
			gNativeMulticlassState.profile_name.c_str()
		);
		NativeMulticlassSetLabel(TrioLabel, trio);

		char presentation[256];
		if (gNativeMulticlassState.max_mana > 0) {
			sprintf_s(
				presentation,
				"Presentation: %s. Base: %s. Mana: %d/%d. Pets: %s. Reweaves: %d.",
				gNativeMulticlassState.presentation_name.c_str(),
				gNativeMulticlassState.base_name.c_str(),
				gNativeMulticlassState.mana,
				gNativeMulticlassState.max_mana,
				gNativeMulticlassState.multiple_pets ? "enabled" : "disabled",
				gNativeMulticlassState.reweaves
			);
		}
		else {
			sprintf_s(
				presentation,
				"Presentation: %s. Base: %s. Pets: %s. Reweaves: %d.",
				gNativeMulticlassState.presentation_name.c_str(),
				gNativeMulticlassState.base_name.c_str(),
				gNativeMulticlassState.multiple_pets ? "enabled" : "disabled",
				gNativeMulticlassState.reweaves
			);
		}
		NativeMulticlassSetLabel(PresentationLabel, presentation);

		NativeMulticlassSetLabel(Slot2Label, "Second Class");
		NativeMulticlassSetLabel(Slot3Label, "Third Class");
		NativeMulticlassSetLabel(Slot2ValueLabel, NativeMulticlassClassName(gNativeMulticlassState.selected_slot2));
		NativeMulticlassSetLabel(Slot3ValueLabel, NativeMulticlassClassName(gNativeMulticlassState.selected_slot3));

		const bool editable = !gNativeMulticlassState.locked && gNativeMulticlassState.can_choose;
		NativeMulticlassSetVisible((CXWnd*)Slot2PrevButton, editable);
		NativeMulticlassSetVisible((CXWnd*)Slot2NextButton, editable);
		NativeMulticlassSetVisible((CXWnd*)Slot3PrevButton, editable);
		NativeMulticlassSetVisible((CXWnd*)Slot3NextButton, editable);
		NativeMulticlassSetVisible((CXWnd*)MelodyButton, gNativeMulticlassState.has_bard);
		NativeMulticlassSetVisible((CXWnd*)DiscsButton, gNativeMulticlassState.locked);
		NativeMulticlassSetButtonText(LockButton, gNativeMulticlassState.locked ? "Locked" : "Lock Trio");

		char info_header[192];
		sprintf_s(info_header, "Trio Notes - %s", gNativeMulticlassState.resonance.c_str());
		NativeMulticlassSetLabel(InfoHeaderLabel, info_header);

		char info[768];
		sprintf_s(
			info,
			"%s Roles: %s. %s %s",
			gNativeMulticlassState.summary.c_str(),
			gNativeMulticlassState.roles.c_str(),
			gNativeMulticlassState.skill_summary.c_str(),
			gNativeMulticlassState.bonus_summary.c_str()
		);
		NativeMulticlassSetLabel(InfoLabel, info);
	}

private:
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CXWnd* TrioLabel = nullptr;
	CXWnd* PresentationLabel = nullptr;
	CXWnd* Slot2Label = nullptr;
	CButtonWnd* Slot2PrevButton = nullptr;
	CXWnd* Slot2ValueLabel = nullptr;
	CButtonWnd* Slot2NextButton = nullptr;
	CXWnd* Slot3Label = nullptr;
	CButtonWnd* Slot3PrevButton = nullptr;
	CXWnd* Slot3ValueLabel = nullptr;
	CButtonWnd* Slot3NextButton = nullptr;
	CButtonWnd* MelodyButton = nullptr;
	CButtonWnd* DiscsButton = nullptr;
	CButtonWnd* PetsButton = nullptr;
	CButtonWnd* LockButton = nullptr;
	CXWnd* InfoHeaderLabel = nullptr;
	CXWnd* InfoLabel = nullptr;
};

class NativeMulticlassPetWnd : public CCustomWnd
{
public:
	NativeMulticlassPetWnd() : CCustomWnd((char*)"NativeMulticlassPetWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeMulticlassPetWnd);

		StatusLabel = GetChildItem("MCPW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("MCPW_RefreshButton");
		PetList = (CListWnd*)GetChildItem("MCPW_PetList");
		AttackButton = (CButtonWnd*)GetChildItem("MCPW_PetAttackButton");
		BackButton = (CButtonWnd*)GetChildItem("MCPW_PetBackButton");
		FollowButton = (CButtonWnd*)GetChildItem("MCPW_PetFollowButton");
		GuardButton = (CButtonWnd*)GetChildItem("MCPW_PetGuardButton");
		HealthButton = (CButtonWnd*)GetChildItem("MCPW_PetHealthButton");
		TauntButton = (CButtonWnd*)GetChildItem("MCPW_PetTauntButton");
		SitButton = (CButtonWnd*)GetChildItem("MCPW_PetSitButton");
		HoldButton = (CButtonWnd*)GetChildItem("MCPW_PetHoldButton");
		SpellHoldButton = (CButtonWnd*)GetChildItem("MCPW_PetSpellHoldButton");
		DismissButton = (CButtonWnd*)GetChildItem("MCPW_PetDismissButton");

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
			if (pWnd == (CXWnd*)PetList) {
				const int selected_pet_id = SelectedPetID();
				if (selected_pet_id > 0) {
					char command[128];
					sprintf_s(command, "/say #mc pet focus %d", selected_pet_id);
					NativeAutoLootSendCommand(command);
					SetStatus("Focusing pet...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)RefreshButton) {
				NativeAutoLootSendCommand("/say #mc pet refresh");
				SetStatus("Refreshing pet roster...");
				return 1;
			}

			if (pWnd == (CXWnd*)AttackButton) {
				SendAction("attack", true);
				return 1;
			}

			if (pWnd == (CXWnd*)BackButton) {
				SendAction("back", true);
				return 1;
			}

			if (pWnd == (CXWnd*)FollowButton) {
				SendAction("follow", true);
				return 1;
			}

			if (pWnd == (CXWnd*)GuardButton) {
				SendAction("guard", true);
				return 1;
			}

			if (pWnd == (CXWnd*)HealthButton) {
				SendAction("health", true);
				return 1;
			}

			if (pWnd == (CXWnd*)TauntButton) {
				SendAction("taunt", false);
				return 1;
			}

			if (pWnd == (CXWnd*)SitButton) {
				SendAction("sit", false);
				return 1;
			}

			if (pWnd == (CXWnd*)HoldButton) {
				SendAction("hold", false);
				return 1;
			}

			if (pWnd == (CXWnd*)SpellHoldButton) {
				SendAction("spellhold", false);
				return 1;
			}

			if (pWnd == (CXWnd*)DismissButton) {
				SendAction("dismiss", false);
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
	}

	void SetStatus(const char* text)
	{
		NativeMulticlassSetLabel(StatusLabel, text);
	}

	void RefreshRows()
	{
		if (!PetList) {
			return;
		}

		PetList->DeleteAll();

		if (SelectedPetIDValue <= 0) {
			SelectedPetIDValue = gNativeMulticlassState.focus_id;
		}

		if (!NativeMulticlassFindPet(SelectedPetIDValue) && !gNativeMulticlassPets.empty()) {
			SelectedPetIDValue = gNativeMulticlassState.focus_id > 0 ? gNativeMulticlassState.focus_id : gNativeMulticlassPets.front().id;
		}

		int selected_row = -1;
		for (const auto& pet : gNativeMulticlassPets) {
			char name[128];
			sprintf_s(name, "%s%s", pet.focused ? "* " : "", pet.name.c_str());
			CXStr name_text(name);
			const COLORREF row_color = pet.id == SelectedPetIDValue ? 0xFFFF4040 : 0xFFFFFFFF;
			const int row = PetList->AddString(name_text, row_color, (uint32_t)pet.id, nullptr, nullptr);

			char hp_mp[32];
			sprintf_s(hp_mp, "%d/%d", pet.hp, pet.mana);
			char toggles[32];
			sprintf_s(toggles, "%c/%c/%c", pet.taunt ? 'Y' : '-', pet.hold ? 'Y' : '-', pet.spellhold ? 'Y' : '-');
			CXStr order(pet.order.c_str());
			CXStr hpmp(hp_mp);
			CXStr toggle_text(toggles);
			PetList->SetItemText(row, 1, &order);
			PetList->SetItemText(row, 2, &hpmp);
			PetList->SetItemText(row, 3, &toggle_text);

			if (pet.id == SelectedPetIDValue) {
				selected_row = row;
			}
		}

		if (gNativeMulticlassPets.empty()) {
			CXStr dash("-");
			const int row = PetList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
			CXStr empty("No active roster.");
			PetList->SetItemText(row, 1, &empty);
			PetList->SetItemText(row, 2, &dash);
			PetList->SetItemText(row, 3, &dash);
		}
		else if (selected_row >= 0) {
			PetList->SetCurSel(selected_row);
		}

		char status[160];
		const NativeMulticlassPetRow* focused = NativeMulticlassFindPet(gNativeMulticlassState.focus_id);
		sprintf_s(
			status,
			"Pets %d/%d. Focus: %s.",
			gNativeMulticlassState.roster_count,
			gNativeMulticlassState.roster_limit,
			focused ? focused->name.c_str() : "-"
		);
		SetStatus(status);
	}

private:
	int SelectedPetID()
	{
		if (!PetList) {
			return SelectedPetIDValue;
		}

		const int selected = PetList->GetCurSel();
		if (selected < 0) {
			return SelectedPetIDValue;
		}

		const int pet_id = (int)PetList->GetItemData(selected);
		if (pet_id > 0) {
			SelectedPetIDValue = pet_id;
		}

		return SelectedPetIDValue;
	}

	void SendAction(const char* action, bool all_pets)
	{
		if (!action || !action[0]) {
			return;
		}

		char command[160];
		if (all_pets) {
			sprintf_s(command, "/say #mc pet %s all", action);
		}
		else {
			const int pet_id = SelectedPetID();
			if (pet_id > 0) {
				sprintf_s(command, "/say #mc pet %s %d", action, pet_id);
			}
			else {
				sprintf_s(command, "/say #mc pet %s", action);
			}
		}

		NativeAutoLootSendCommand(command);
		SetStatus("Sending pet command...");
	}

	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CListWnd* PetList = nullptr;
	CButtonWnd* AttackButton = nullptr;
	CButtonWnd* BackButton = nullptr;
	CButtonWnd* FollowButton = nullptr;
	CButtonWnd* GuardButton = nullptr;
	CButtonWnd* HealthButton = nullptr;
	CButtonWnd* TauntButton = nullptr;
	CButtonWnd* SitButton = nullptr;
	CButtonWnd* HoldButton = nullptr;
	CButtonWnd* SpellHoldButton = nullptr;
	CButtonWnd* DismissButton = nullptr;
	int SelectedPetIDValue = 0;
};

class NativeMulticlassMelodyWnd : public CCustomWnd
{
public:
	NativeMulticlassMelodyWnd() : CCustomWnd((char*)"NativeMulticlassMelodyWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeMulticlassMelodyWnd);

		StatusLabel = GetChildItem("MCMW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("MCMW_RefreshButton");
		SlotList = (CListWnd*)GetChildItem("MCMW_SlotList");
		SongList = (CListWnd*)GetChildItem("MCMW_SongList");
		PrevButton = (CButtonWnd*)GetChildItem("MCMW_PrevButton");
		NextButton = (CButtonWnd*)GetChildItem("MCMW_NextButton");
		ClearSlotButton = (CButtonWnd*)GetChildItem("MCMW_ClearSlotButton");
		ClearAllButton = (CButtonWnd*)GetChildItem("MCMW_ClearAllButton");
		FooterLabel = GetChildItem("MCMW_FooterLabel");

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
				NativeAutoLootSendCommand("/say #mc melody refresh");
				SetStatus("Refreshing Bard Melody...");
				return 1;
			}

			if (pWnd == (CXWnd*)SlotList) {
				SelectedSlot();
				RefreshRows();
				return 1;
			}

			if (pWnd == (CXWnd*)SongList) {
				const int spell_id = SelectedSongID();
				const int slot = SelectedSlot();
				if (slot > 0 && spell_id > 0) {
					SendSet(slot, spell_id);
				}
				return 1;
			}

			if (pWnd == (CXWnd*)PrevButton) {
				CycleSong(-1);
				return 1;
			}

			if (pWnd == (CXWnd*)NextButton) {
				CycleSong(1);
				return 1;
			}

			if (pWnd == (CXWnd*)ClearSlotButton) {
				const int slot = SelectedSlot();
				if (slot > 0) {
					char command[128];
					sprintf_s(command, "/say #mc melody clear %d", slot);
					NativeAutoLootSendCommand(command);
					SetStatus("Clearing melody slot...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)ClearAllButton) {
				NativeAutoLootSendCommand("/say #mc melody clear");
				SetStatus("Clearing Bard Melody...");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
	}

	void SetStatus(const char* text)
	{
		NativeMulticlassSetLabel(StatusLabel, text);
	}

	void RefreshRows()
	{
		if (!SlotList || !SongList) {
			return;
		}

		SlotList->DeleteAll();
		SongList->DeleteAll();

		if (SelectedSlotValue < 1 || SelectedSlotValue > 4) {
			SelectedSlotValue = 1;
		}

		int selected_slot_row = -1;
		for (int slot_id = 1; slot_id <= 4; ++slot_id) {
			const NativeMulticlassMelodySlot* slot = FindSlot(slot_id);
			char label[24];
			sprintf_s(label, "Slot %d", slot_id);
			CXStr slot_text(label);
			const COLORREF row_color = slot_id == SelectedSlotValue ? 0xFFFF4040 : 0xFFFFFFFF;
			const int row = SlotList->AddString(slot_text, row_color, (uint32_t)slot_id, nullptr, nullptr);

			CXStr song(slot ? slot->name.c_str() : "-");
			char level[32];
			sprintf_s(level, "%d", slot ? slot->level : 0);
			CXStr level_text(slot && slot->level > 0 ? level : "-");
			CXStr state(slot ? slot->state.c_str() : "empty");
			SlotList->SetItemText(row, 1, &song);
			SlotList->SetItemText(row, 2, &level_text);
			SlotList->SetItemText(row, 3, &state);

			if (slot_id == SelectedSlotValue) {
				selected_slot_row = row;
			}
		}

		if (selected_slot_row >= 0) {
			SlotList->SetCurSel(selected_slot_row);
		}

		int selected_song_row = -1;
		for (const auto& song : gNativeMulticlassMelodySongs) {
			CXStr name(song.name.c_str());
			const COLORREF row_color = song.allowed ? 0xFFFFFFFF : 0xFF909090;
			const int row = SongList->AddString(name, row_color, (uint32_t)song.spell_id, nullptr, nullptr);

			char level[32];
			sprintf_s(level, "%d", song.level);
			CXStr level_text(level);
			CXStr state(song.allowed ? "Ready" : "Blocked");
			CXStr reason(song.reason.empty() ? "-" : song.reason.c_str());
			SongList->SetItemText(row, 1, &level_text);
			SongList->SetItemText(row, 2, &state);
			SongList->SetItemText(row, 3, &reason);

			if (song.spell_id == SelectedSongIDValue) {
				selected_song_row = row;
			}
		}

		if (gNativeMulticlassMelodySongs.empty()) {
			CXStr dash("-");
			const int row = SongList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
			CXStr empty(gNativeMulticlassState.has_bard ? "No scribed Bard songs." : "Bard not in trio.");
			SongList->SetItemText(row, 1, &dash);
			SongList->SetItemText(row, 2, &empty);
			SongList->SetItemText(row, 3, &dash);
		}
		else if (selected_song_row >= 0) {
			SongList->SetCurSel(selected_song_row);
		}

		SetStatus(gNativeMulticlassState.melody_status.c_str());
		NativeMulticlassSetLabel(FooterLabel, "Select a slot, then choose a scribed Bard song. Spell gems stay free.");
	}

private:
	const NativeMulticlassMelodySlot* FindSlot(int slot_id) const
	{
		for (const auto& slot : gNativeMulticlassMelodySlots) {
			if (slot.slot == slot_id) {
				return &slot;
			}
		}

		return nullptr;
	}

	int SelectedSlot()
	{
		if (!SlotList) {
			return SelectedSlotValue;
		}

		const int selected = SlotList->GetCurSel();
		if (selected >= 0) {
			const int slot_id = (int)SlotList->GetItemData(selected);
			if (slot_id >= 1 && slot_id <= 4) {
				SelectedSlotValue = slot_id;
			}
		}

		return SelectedSlotValue;
	}

	int SelectedSongID()
	{
		if (!SongList) {
			return SelectedSongIDValue;
		}

		const int selected = SongList->GetCurSel();
		if (selected >= 0) {
			const int spell_id = (int)SongList->GetItemData(selected);
			if (spell_id > 0) {
				SelectedSongIDValue = spell_id;
			}
		}

		return SelectedSongIDValue;
	}

	void SendSet(int slot_id, int spell_id)
	{
		char command[160];
		sprintf_s(command, "/say #mc melody set %d %d", slot_id, spell_id);
		NativeAutoLootSendCommand(command);
		SetStatus("Setting melody slot...");
	}

	void CycleSong(int direction)
	{
		if (gNativeMulticlassMelodySongs.empty()) {
			SetStatus("No Bard songs are available.");
			return;
		}

		std::vector<int> allowed_indices;
		for (size_t i = 0; i < gNativeMulticlassMelodySongs.size(); ++i) {
			if (gNativeMulticlassMelodySongs[i].allowed) {
				allowed_indices.push_back((int)i);
			}
		}

		if (allowed_indices.empty()) {
			SetStatus("No Bard songs are ready for Melody.");
			return;
		}

		int current_allowed_index = -1;
		for (size_t i = 0; i < allowed_indices.size(); ++i) {
			if (gNativeMulticlassMelodySongs[allowed_indices[i]].spell_id == SelectedSongIDValue) {
				current_allowed_index = (int)i;
				break;
			}
		}

		if (current_allowed_index < 0) {
			current_allowed_index = 0;
		}
		else {
			current_allowed_index += direction;
			if (current_allowed_index < 0) {
				current_allowed_index = (int)allowed_indices.size() - 1;
			}
			if (current_allowed_index >= (int)allowed_indices.size()) {
				current_allowed_index = 0;
			}
		}

		const auto& song = gNativeMulticlassMelodySongs[allowed_indices[current_allowed_index]];
		SelectedSongIDValue = song.spell_id;
		SendSet(SelectedSlot(), song.spell_id);
		RefreshRows();
	}

	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CListWnd* SlotList = nullptr;
	CListWnd* SongList = nullptr;
	CButtonWnd* PrevButton = nullptr;
	CButtonWnd* NextButton = nullptr;
	CButtonWnd* ClearSlotButton = nullptr;
	CButtonWnd* ClearAllButton = nullptr;
	CXWnd* FooterLabel = nullptr;
	int SelectedSlotValue = 1;
	int SelectedSongIDValue = 0;
};

class NativeMulticlassDisciplineWnd : public CCustomWnd
{
public:
	NativeMulticlassDisciplineWnd() : CCustomWnd((char*)"NativeMulticlassDisciplineWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeMulticlassDisciplineWnd);

		StatusLabel = GetChildItem("MCDW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("MCDW_RefreshButton");
		ActiveEffectLabel = GetChildItem("MCDW_ActiveEffectLabel");
		ActiveEffectGauge = (CGaugeWnd*)GetChildItem("MCDW_ActiveEffectGauge");
		ReuseLabel = GetChildItem("MCDW_ReuseLabel");
		DisciplineList = (CListWnd*)GetChildItem("MCDW_DisciplineList");
		UseButton = (CButtonWnd*)GetChildItem("MCDW_UseButton");
		HotkeyButton = (CButtonWnd*)GetChildItem("MCDW_HotkeyButton");
		FooterLabel = GetChildItem("MCDW_FooterLabel");

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
				NativeAutoLootSendCommand("/say #mc disc refresh");
				SetStatus("Refreshing disciplines...");
				return 1;
			}

			if (pWnd == (CXWnd*)DisciplineList) {
				SelectedSpellID();
				RefreshRows();
				return 1;
			}

			if (pWnd == (CXWnd*)UseButton) {
				SendUse();
				return 1;
			}

			if (pWnd == (CXWnd*)HotkeyButton) {
				CreateHotkey();
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
		// Resize is handled by SIDL AutoStretch anchors. Keep the method so
		// every native Multiclass window has a pulse-time resize hook.
		DWORD now = GetTickCount();
		if (now - LastTimerRefreshMs >= 1000) {
			LastTimerRefreshMs = now;
			if (HasActiveReuseTimer()) {
				RefreshRows();
			}
			else {
				UpdateTimerLabels();
			}
		}
	}

	void SetStatus(const char* text)
	{
		NativeMulticlassSetLabel(StatusLabel, text);
	}

	void RefreshRows()
	{
		if (!DisciplineList) {
			return;
		}

		DisciplineList->DeleteAll();

		if (SelectedSpellIDValue > 0 && !FindDiscipline(SelectedSpellIDValue)) {
			SelectedSpellIDValue = 0;
		}

		if (SelectedSpellIDValue <= 0 && !gNativeMulticlassDisciplineRows.empty()) {
			SelectedSpellIDValue = gNativeMulticlassDisciplineRows.front().spell_id;
		}

		int selected_row = -1;
		for (const auto& disc : gNativeMulticlassDisciplineRows) {
			CXStr name(disc.name.c_str());
			COLORREF row_color = disc.ready ? 0xFFFFFFFF : 0xFFB0B0B0;
			if (disc.spell_id == SelectedSpellIDValue) {
				row_color = 0xFFFF4040;
			}

			const int row = DisciplineList->AddString(name, row_color, (uint32_t)disc.spell_id, nullptr, nullptr);

			char level[32];
			sprintf_s(level, "%d", disc.level);
			char timer[32];
			const int remaining_timer = CurrentTimerSeconds(disc);
			const bool locally_ready = disc.ready || (disc.timer > 0 && remaining_timer <= 0);
			if (remaining_timer > 0) {
				const std::string formatted_timer = FormatTimer(remaining_timer);
				strcpy_s(timer, sizeof(timer), formatted_timer.c_str());
			}
			else {
				sprintf_s(timer, "-");
			}

			CXStr level_text(level);
			CXStr timer_text(timer);
			const std::string state = locally_ready ? "Ready" : (remaining_timer > 0 ? "Reuse" : disc.state);
			CXStr state_text(state.c_str());
			DisciplineList->SetItemText(row, 1, &level_text);
			DisciplineList->SetItemText(row, 2, &timer_text);
			DisciplineList->SetItemText(row, 3, &state_text);

			if (disc.spell_id == SelectedSpellIDValue) {
				selected_row = row;
			}
		}

		if (gNativeMulticlassDisciplineRows.empty()) {
			CXStr dash("-");
			const int row = DisciplineList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
			CXStr empty("No learned disciplines.");
			DisciplineList->SetItemText(row, 1, &dash);
			DisciplineList->SetItemText(row, 2, &dash);
			DisciplineList->SetItemText(row, 3, &empty);
		}
		else if (selected_row >= 0) {
			DisciplineList->SetCurSel(selected_row);
		}

		SetStatus(gNativeMulticlassState.discipline_status.c_str());
		UpdateTimerLabels();
		NativeMulticlassSetLabel(FooterLabel, "Use activates selected disc. Hotkey creates a /mc disc button.");
	}

private:
	const NativeMulticlassDisciplineRow* FindDiscipline(int spell_id) const
	{
		for (const auto& disc : gNativeMulticlassDisciplineRows) {
			if (disc.spell_id == spell_id) {
				return &disc;
			}
		}

		return nullptr;
	}

	int SelectedSpellID()
	{
		if (!DisciplineList) {
			return SelectedSpellIDValue;
		}

		const int selected = DisciplineList->GetCurSel();
		if (selected >= 0) {
			const int spell_id = (int)DisciplineList->GetItemData(selected);
			if (spell_id > 0) {
				SelectedSpellIDValue = spell_id;
			}
		}

		return SelectedSpellIDValue;
	}

	void SendUse()
	{
		const int spell_id = SelectedSpellID();
		if (spell_id <= 0) {
			SetStatus("Select a discipline first.");
			return;
		}

		char command[128];
		sprintf_s(command, "/say #mc disc use %d", spell_id);
		NativeAutoLootSendCommand(command);
		SetStatus("Using discipline...");
	}

	void CreateHotkey()
	{
		const int spell_id = SelectedSpellID();
		const auto* disc = FindDiscipline(spell_id);
		if (!disc) {
			SetStatus("Select a discipline first.");
			return;
		}

		const std::string label = HotkeyLabel(disc->name, spell_id);
		char command[256];
		sprintf_s(command, "/hotbutton %s /mc disc use %d", label.c_str(), spell_id);
		NativeAutoLootSendCommand(command);

		char status[256];
		sprintf_s(status, "Created hotkey for %.180s.", disc->name.c_str());
		SetStatus(status);
	}

	static int CurrentTimerSeconds(const NativeMulticlassDisciplineRow& disc)
	{
		if (disc.timer <= 0) {
			return 0;
		}

		const DWORD elapsed_ms = disc.timer_received_ms ? (GetTickCount() - disc.timer_received_ms) : 0;
		const int elapsed_seconds = static_cast<int>(elapsed_ms / 1000);
		if (elapsed_seconds >= disc.timer) {
			return 0;
		}

		return disc.timer - elapsed_seconds;
	}

	static std::string FormatTimer(int seconds)
	{
		if (seconds <= 0) {
			return "-";
		}

		char buffer[32];
		if (seconds >= 3600) {
			sprintf_s(buffer, "%d:%02d:%02d", seconds / 3600, (seconds / 60) % 60, seconds % 60);
		}
		else if (seconds >= 60) {
			sprintf_s(buffer, "%d:%02d", seconds / 60, seconds % 60);
		}
		else {
			sprintf_s(buffer, "%ds", seconds);
		}

		return buffer;
	}

	static std::string ReuseBar(int remaining, int total)
	{
		if (remaining <= 0 || total <= 0) {
			return "Ready";
		}

		const int width = 12;
		int filled = (remaining * width + total - 1) / total;
		if (filled < 1) {
			filled = 1;
		}
		if (filled > width) {
			filled = width;
		}

		std::string bar = "[";
		for (int i = 0; i < width; ++i) {
			bar.push_back(i < filled ? '#' : '-');
		}
		bar.push_back(']');
		return bar;
	}

	static std::string HotkeyLabel(const std::string& name, int spell_id)
	{
		std::string label;
		label.reserve(16);
		for (char c : name) {
			if (label.size() >= 15) {
				break;
			}

			const unsigned char ch = static_cast<unsigned char>(c);
			if (isalnum(ch)) {
				label.push_back(static_cast<char>(c));
			}
			else if ((isspace(ch) || c == '-' || c == '_') && !label.empty() && label.back() != '_') {
				label.push_back('_');
			}
		}

		while (!label.empty() && label.back() == '_') {
			label.resize(label.size() - 1);
		}

		if (label.empty()) {
			char fallback[32];
			sprintf_s(fallback, "Disc%d", spell_id);
			label = fallback;
		}

		return label;
	}

	bool HasActiveReuseTimer() const
	{
		for (const auto& disc : gNativeMulticlassDisciplineRows) {
			if (CurrentTimerSeconds(disc) > 0) {
				return true;
			}
		}

		return false;
	}

	void UpdateTimerLabels()
	{
		NativeMulticlassSetLabel(ActiveEffectLabel, "Active:");
		const auto* disc = FindDiscipline(SelectedSpellIDValue);
		if (!disc) {
			NativeMulticlassSetLabel(ReuseLabel, "Reuse: -");
			return;
		}

		const int remaining = CurrentTimerSeconds(*disc);
		const int total = disc->timer_total > 0 ? disc->timer_total : disc->timer;
		const std::string reuse = remaining > 0 ?
			("Reuse: " + ReuseBar(remaining, total) + " " + FormatTimer(remaining)) :
			"Reuse: ready";
		NativeMulticlassSetLabel(ReuseLabel, reuse.c_str());
	}

	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CXWnd* ActiveEffectLabel = nullptr;
	CGaugeWnd* ActiveEffectGauge = nullptr;
	CXWnd* ReuseLabel = nullptr;
	CListWnd* DisciplineList = nullptr;
	CButtonWnd* UseButton = nullptr;
	CButtonWnd* HotkeyButton = nullptr;
	CXWnd* FooterLabel = nullptr;
	int SelectedSpellIDValue = 0;
	DWORD LastTimerRefreshMs = 0;
};

static void NativeMulticlassRefreshWindows()
{
	if (gNativeMulticlassWnd) {
		gNativeMulticlassWnd->Refresh();
	}

	if (gNativeMulticlassPetWnd) {
		gNativeMulticlassPetWnd->RefreshRows();
	}

	if (gNativeMulticlassMelodyWnd) {
		gNativeMulticlassMelodyWnd->RefreshRows();
	}

	if (gNativeMulticlassDisciplineWnd) {
		gNativeMulticlassDisciplineWnd->RefreshRows();
	}
}

static void NativeMulticlassEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeMulticlassWnd) {
		NativeAutoLootTrace("creating Multiclass window");
		NativeMulticlassWnd* created_window = nullptr;
		__try {
			created_window = new NativeMulticlassWnd();
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass window construction faulted; check EQUI_NativeMulticlassWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			return;
		}

		gNativeMulticlassWnd = created_window;
		NativeAutoLootTrace("Multiclass window created");
	}

	gNativeMulticlassWnd->Refresh();
	if (show && gNativeMulticlassWnd) {
		gNativeMulticlassWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeMulticlassEnsurePetWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeMulticlassPetWnd) {
		NativeAutoLootTrace("creating Multiclass pet window");
		NativeMulticlassPetWnd* created_window = nullptr;
		__try {
			created_window = new NativeMulticlassPetWnd();
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass pet window construction faulted; check EQUI_NativeMulticlassWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			return;
		}

		gNativeMulticlassPetWnd = created_window;
		NativeAutoLootTrace("Multiclass pet window created");
	}

	gNativeMulticlassPetWnd->RefreshRows();
	if (show && gNativeMulticlassPetWnd) {
		gNativeMulticlassPetWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeMulticlassEnsureMelodyWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeMulticlassMelodyWnd) {
		NativeAutoLootTrace("creating Multiclass melody window");
		NativeMulticlassMelodyWnd* created_window = nullptr;
		__try {
			created_window = new NativeMulticlassMelodyWnd();
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass melody window construction faulted; check EQUI_NativeMulticlassWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			return;
		}

		gNativeMulticlassMelodyWnd = created_window;
		NativeAutoLootTrace("Multiclass melody window created");
	}

	gNativeMulticlassMelodyWnd->RefreshRows();
	if (show && gNativeMulticlassMelodyWnd) {
		gNativeMulticlassMelodyWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeMulticlassEnsureDisciplineWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeMulticlassDisciplineWnd) {
		NativeAutoLootTrace("creating Multiclass discipline window");
		NativeMulticlassDisciplineWnd* created_window = nullptr;
		__try {
			created_window = new NativeMulticlassDisciplineWnd();
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass discipline window construction faulted; check EQUI_NativeMulticlassWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			return;
		}

		gNativeMulticlassDisciplineWnd = created_window;
		NativeAutoLootTrace("Multiclass discipline window created");
	}

	gNativeMulticlassDisciplineWnd->RefreshRows();
	if (show && gNativeMulticlassDisciplineWnd) {
		gNativeMulticlassDisciplineWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeMulticlassHideRuntimeWindows()
{
	__try {
		if (gNativeMulticlassWnd) {
			gNativeMulticlassWnd->pXWnd()->Show(0, 1);
		}
		if (gNativeMulticlassPetWnd) {
			gNativeMulticlassPetWnd->pXWnd()->Show(0, 1);
		}
		if (gNativeMulticlassMelodyWnd) {
			gNativeMulticlassMelodyWnd->pXWnd()->Show(0, 1);
		}
		if (gNativeMulticlassDisciplineWnd) {
			gNativeMulticlassDisciplineWnd->pXWnd()->Show(0, 1);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
}

static void NativeMulticlassDestroyRuntimeWindows()
{
	if (gNativeMulticlassWnd) {
		NativeMulticlassWnd* window = gNativeMulticlassWnd;
		gNativeMulticlassWnd = nullptr;
		__try {
			window->pXWnd()->Show(0, 1);
			delete window;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass window release faulted during UI reset");
		}
	}

	if (gNativeMulticlassPetWnd) {
		NativeMulticlassPetWnd* window = gNativeMulticlassPetWnd;
		gNativeMulticlassPetWnd = nullptr;
		__try {
			window->pXWnd()->Show(0, 1);
			delete window;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass pet window release faulted during UI reset");
		}
	}

	if (gNativeMulticlassMelodyWnd) {
		NativeMulticlassMelodyWnd* window = gNativeMulticlassMelodyWnd;
		gNativeMulticlassMelodyWnd = nullptr;
		__try {
			window->pXWnd()->Show(0, 1);
			delete window;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass melody window release faulted during UI reset");
		}
	}

	if (gNativeMulticlassDisciplineWnd) {
		NativeMulticlassDisciplineWnd* window = gNativeMulticlassDisciplineWnd;
		gNativeMulticlassDisciplineWnd = nullptr;
		__try {
			window->pXWnd()->Show(0, 1);
			delete window;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("Multiclass discipline window release faulted during UI reset");
		}
	}
}

static void NativeMulticlassResetSessionState(bool hide_windows)
{
	if (hide_windows) {
		NativeMulticlassHideRuntimeWindows();
	}

	gNativeMulticlassState = NativeMulticlassState();
	gNativeMulticlassPets.clear();
	gNativeMulticlassMelodySlots.clear();
	gNativeMulticlassMelodySongs.clear();
	gNativeMulticlassDisciplineRows.clear();
	gNativeMulticlassSpellLevelsById.clear();
	gNativeMulticlassSpellLevelsByName.clear();
	gNativeMulticlassSpellLevelPatchCount = 0;
	gNativeMulticlassSpellLevelsLoading = false;
	gNativeMulticlassSpellLevelReapplyDelay = 0;
	gNativeMulticlassSpellLevelReapplyPasses = 0;
	gNativeMulticlassShowCasterUiPulses = 0;
}

class NativeMulticlassContextMenuHook
{
public:
	int Trampoline(const CXStr& text, unsigned int menu_id, bool checked, DWORD color, bool enabled);
	int Detour(const CXStr& text, unsigned int menu_id, bool checked, DWORD color, bool enabled)
	{
		const char* raw_text = text.Ptr ? text.Ptr->Text : "";
		std::string rewritten;
		if (NativeMulticlassRewriteSpellMenuText(raw_text, menu_id, rewritten)) {
			CXStr value(rewritten.c_str());
			return Trampoline(value, menu_id, checked, color, enabled);
		}

		return Trampoline(text, menu_id, checked, color, enabled);
	}
};

DETOUR_TRAMPOLINE_EMPTY(int NativeMulticlassContextMenuHook::Trampoline(const CXStr& text, unsigned int menu_id, bool checked, DWORD color, bool enabled));

static DWORD NativeMulticlassContextMenuAddMenuItemAddress()
{
	return (((DWORD)0x0085B7B0 - 0x400000) + baseAddress);
}

static void NativeMulticlassInstallContextMenuHook()
{
	if (!gNativeMulticlassContextMenuHookEnabled || gNativeMulticlassContextMenuHookInstalled) {
		return;
	}

	NativeAutoLootTrace("installing Multiclass context menu hook");
	EzDetour(NativeMulticlassContextMenuAddMenuItemAddress(), &NativeMulticlassContextMenuHook::Detour, &NativeMulticlassContextMenuHook::Trampoline);
	gNativeMulticlassContextMenuHookInstalled = true;
}

static bool NativeIsKeepRule(const std::string& rule)
{
	return rule == "always_need" || rule == "always_greed" || rule == "include" || rule == "keep" || rule == "always" || rule == "loot";
}

static bool NativeIsNeverRule(const std::string& rule)
{
	return rule == "exclude" || rule == "ignore" || rule == "never" || rule == "skip";
}

static const char* NativeShortRule(const std::string& rule)
{
	if (rule == "always_need") {
		return "AN";
	}

	if (rule == "always_greed") {
		return "AG";
	}

	if (NativeIsNeverRule(rule)) {
		return "NV";
	}

	if (NativeIsKeepRule(rule)) {
		return "X";
	}

	return "";
}

static const char* NativeDisplayRule(const std::string& rule)
{
	if (rule == "always_need") {
		return "AN";
	}

	if (rule == "always_greed") {
		return "AG";
	}

	if (NativeIsNeverRule(rule)) {
		return "NV";
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

struct NativeAchievementRewardRow
{
	uint64_t definition_id = 0;
	int reward_id = 0;
	int amount = 0;
	bool auto_claim = false;
	std::string type;
	std::string tier;
	std::string name;
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
		RewardList = (CListWnd*)GetChildItem("NAW_RewardList");
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
	void RefreshRewardList();
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
	CListWnd* RewardList = nullptr;
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
static std::vector<NativeAchievementRewardRow> gNativeAchievementRewards;
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
static bool gNativeAchievementRewardsDirty = true;
static std::string gNativeAchievementDetailTitle = "Select an achievement";
static std::string gNativeAchievementDetailDescription = "";

struct NativeFactionRow
{
	int id = 0;
	int raw_value = 0;
	int modified_value = 0;
	bool touched = false;
	bool target = false;
	bool pinned = false;
	bool hidden = false;
	std::string name = "Faction";
	std::string standing = "Indifferently";
	std::string section = "All";
};

static bool NativeFactionRowMatchesSearch(const NativeFactionRow& faction, const std::string& search_lower)
{
	if (search_lower.empty()) {
		return true;
	}

	char id_text[32];
	sprintf_s(id_text, "%d", faction.id);
	if (strstr(id_text, search_lower.c_str())) {
		return true;
	}

	if (
		NativeContainsLower(faction.name, search_lower) ||
		NativeContainsLower(faction.standing, search_lower) ||
		NativeContainsLower(faction.section, search_lower)
	) {
		return true;
	}

	if (faction.target && NativeContainsLower("target", search_lower)) {
		return true;
	}

	if (faction.pinned && NativeContainsLower("pinned", search_lower)) {
		return true;
	}

	if (faction.hidden && NativeContainsLower("hidden", search_lower)) {
		return true;
	}

	if (faction.touched && NativeContainsLower("changed", search_lower)) {
		return true;
	}

	return false;
}

struct NativeDpsRow
{
	int actor_id = 0;
	int owner_id = 0;
	unsigned long long damage = 0;
	unsigned long long healing = 0;
	unsigned long long incoming = 0;
	unsigned long long dps = 0;
	unsigned long long hps = 0;
	int pct = 0;
	std::string actor = "Actor";
	std::string source = "Source";
};

class NativeDpsWnd : public CCustomWnd
{
public:
	NativeDpsWnd() : CCustomWnd((char*)"NativeDpsWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeDpsWnd);

		SummaryLabel = GetChildItem("NDPS_SummaryLabel");
		DpsList = (CListWnd*)GetChildItem("NDPS_DpsList");
		StatusLabel = GetChildItem("NDPS_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("NDPS_RefreshButton");
		ResetButton = (CButtonWnd*)GetChildItem("NDPS_ResetButton");
		LiveButton = (CButtonWnd*)GetChildItem("NDPS_LiveButton");

		SetStatus("Waiting for DPS data.");
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
				NativeAutoLootSendCommand("/say #dps live on");
				SetStatus("Refreshing live DPS parser...");
				return 1;
			}

			if (pWnd == (CXWnd*)ResetButton) {
				NativeAutoLootSendCommand("/say #dps reset");
				SetStatus("Resetting DPS parser...");
				return 1;
			}

			if (pWnd == (CXWnd*)LiveButton) {
				NativeAutoLootSendCommand("/say #dps live on");
				SetStatus("Requesting live DPS updates...");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
	}

	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text ? text : "");
	}

	void RefreshRows();

private:
	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	CXWnd* SummaryLabel = nullptr;
	CListWnd* DpsList = nullptr;
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* ResetButton = nullptr;
	CButtonWnd* LiveButton = nullptr;
};

class NativeFactionWnd : public CCustomWnd
{
public:
	NativeFactionWnd() : CCustomWnd((char*)"NativeFactionWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeFactionWnd);

		SummaryLabel = GetChildItem("NFW_SummaryLabel");
		FactionList = (CListWnd*)GetChildItem("NFW_FactionList");
		StatusLabel = GetChildItem("NFW_StatusLabel");
		RefreshButton = (CButtonWnd*)GetChildItem("NFW_RefreshButton");
		ListButton = (CButtonWnd*)GetChildItem("NFW_ListButton");
		PinButton = (CButtonWnd*)GetChildItem("NFW_PinButton");
		HideButton = (CButtonWnd*)GetChildItem("NFW_HideButton");
		ShowButton = (CButtonWnd*)GetChildItem("NFW_ShowButton");
		HiddenButton = (CButtonWnd*)GetChildItem("NFW_HiddenButton");
		SearchEdit = (CEditWnd*)GetChildItem("NFW_SearchEdit");
		SearchButton = (CButtonWnd*)GetChildItem("NFW_SearchButton");
		ClearSearchButton = (CButtonWnd*)GetChildItem("NFW_ClearSearchButton");

		SetStatus("Use /rep to refresh. Select a row to pin, hide, or unhide it.");
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
				NativeAutoLootSendCommand("/say #rep refresh");
				SetStatus("Refreshing faction reputation...");
				return 1;
			}

			if (pWnd == (CXWnd*)ListButton) {
				NativeAutoLootSendCommand("/say #rep list");
				SetStatus("Printing faction standings to chat...");
				return 1;
			}

			if (pWnd == (CXWnd*)PinButton) {
				const int faction_id = SelectedFactionId();
				if (faction_id > 0) {
					char command[80];
					sprintf_s(command, "/say #rep pin %d", faction_id);
					NativeAutoLootSendCommand(command);
					SetStatus("Pinning selected faction...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)HideButton) {
				const int faction_id = SelectedFactionId();
				if (faction_id > 0) {
					char command[80];
					sprintf_s(command, "/say #rep hide %d", faction_id);
					NativeAutoLootSendCommand(command);
					SetStatus("Hiding selected faction...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)ShowButton) {
				const int faction_id = SelectedFactionId();
				if (faction_id > 0) {
					char command[80];
					sprintf_s(command, "/say #rep show %d", faction_id);
					NativeAutoLootSendCommand(command);
					SetStatus("Unhiding selected faction...");
				}
				return 1;
			}

			if (pWnd == (CXWnd*)HiddenButton) {
				NativeAutoLootSendCommand("/say #rep hidden");
				SetStatus("Loading hidden factions...");
				return 1;
			}

			if (pWnd == (CXWnd*)SearchButton) {
				ApplySearchFromEdit(true);
				return 1;
			}

			if (pWnd == (CXWnd*)ClearSearchButton) {
				ClearSearch();
				return 1;
			}
		}

		if (pWnd == (CXWnd*)SearchEdit && Message == XWM_NEWVALUE) {
			if (!UpdatingSearchText) {
				ApplySearchFromEdit(false, true);
			}
			return 1;
		}

		if (pWnd == (CXWnd*)SearchEdit && Message == XWM_HITENTER) {
			ApplySearchFromEdit(true, true);
			return 1;
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
		if (DeferredSearchRefresh) {
			DeferredSearchRefresh = false;
			RefreshRows();
		}
	}

	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text ? text : "");
	}

	void RefreshRows();

private:
	void ApplySearchFromEdit(bool server_search, bool defer_refresh = false);
	void ClearSearch();
	std::string ReadSearch(bool prefer_input_text = true) const;
	void SetSearchText(const char* text);

	int SelectedFactionId() const
	{
		if (!FactionList) {
			return 0;
		}

		const int selected = FactionList->GetCurSel();
		if (selected < 0) {
			return 0;
		}

		return static_cast<int>(FactionList->GetItemData(selected));
	}

	COLORREF RowColor(const std::string& standing) const
	{
		if (_stricmp(standing.c_str(), "Ally") == 0 || _stricmp(standing.c_str(), "Warmly") == 0) {
			return 0xFF66FF66;
		}

		if (_stricmp(standing.c_str(), "Kindly") == 0 || _stricmp(standing.c_str(), "Amiably") == 0) {
			return 0xFFB8FF8A;
		}

		if (_stricmp(standing.c_str(), "Indifferently") == 0) {
			return 0xFFFFFFFF;
		}

		if (_stricmp(standing.c_str(), "Apprehensively") == 0 || _stricmp(standing.c_str(), "Dubiously") == 0) {
			return 0xFFFFFF80;
		}

		return 0xFFFF9090;
	}

	void SetLabel(CXWnd* label, const char* text)
	{
		if (label) {
			CXStr value(text ? text : "");
			label->SetWindowTextA(value);
		}
	}

	CXWnd* SummaryLabel = nullptr;
	CListWnd* FactionList = nullptr;
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* RefreshButton = nullptr;
	CButtonWnd* ListButton = nullptr;
	CButtonWnd* PinButton = nullptr;
	CButtonWnd* HideButton = nullptr;
	CButtonWnd* ShowButton = nullptr;
	CButtonWnd* HiddenButton = nullptr;
	CEditWnd* SearchEdit = nullptr;
	CButtonWnd* SearchButton = nullptr;
	CButtonWnd* ClearSearchButton = nullptr;
	bool DeferredSearchRefresh = false;
	bool UpdatingSearchText = false;
};

static NativeFactionWnd* gNativeFactionWnd = nullptr;
static std::vector<NativeFactionRow> gNativeFactionRows;
static std::string gNativeFactionStatus = "Use /rep to refresh. Target an NPC to pin its primary faction.";
static int gNativeFactionTargetId = 0;
static std::string gNativeFactionMode = "";
static std::string gNativeFactionSearch = "";
static bool gNativeFactionLoading = false;
static bool gNativeFactionRowsDirty = true;

static NativeDpsWnd* gNativeDpsWnd = nullptr;
static std::vector<NativeDpsRow> gNativeDpsRows;
static std::string gNativeDpsStatus = "Waiting for DPS data.";
static std::string gNativeDpsTarget = "";
static int gNativeDpsEncounterId = 0;
static int gNativeDpsElapsedMs = 0;
static unsigned long long gNativeDpsDamage = 0;
static unsigned long long gNativeDpsHealing = 0;
static unsigned long long gNativeDpsIncoming = 0;
static bool gNativeDpsLoading = false;
static bool gNativeDpsRowsDirty = true;

class NativeTradeskillsWnd : public CCustomWnd
{
public:
	NativeTradeskillsWnd() : CCustomWnd((char*)"NativeTradeskillsWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeTradeskillsWnd);

		StatusLabel = GetChildItem("TRADESKILLS_StatusLabel");
		MakeAllButton = (CButtonWnd*)GetChildItem("TRADESKILLS_MakeAllButton");
		HelpButton = (CButtonWnd*)GetChildItem("TRADESKILLS_HelpButton");

		SetStatus("Select a learned recipe, then use Make All.");
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)MakeAllButton) {
				NativeAutoLootSendCommand("/say #ts makeall");
				SetStatus("Requesting Make All for the selected recipe...");
				return 1;
			}

			if (pWnd == (CXWnd*)HelpButton) {
				NativeAutoLootSendCommand("/say #ts help");
				SetStatus("Printing tradeskill helper commands to chat...");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Layout()
	{
	}

	void SetStatus(const char* text)
	{
		if (StatusLabel) {
			CXStr value(text ? text : "");
			StatusLabel->SetWindowTextA(value);
		}
	}

private:
	CXWnd* StatusLabel = nullptr;
	CButtonWnd* MakeAllButton = nullptr;
	CButtonWnd* HelpButton = nullptr;
};

static NativeTradeskillsWnd* gNativeTradeskillsWnd = nullptr;

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
		Step1Button = (CButtonWnd*)GetChildItem("AIFW_Step1Button");
		Step5Button = (CButtonWnd*)GetChildItem("AIFW_Step5Button");
		Step10Button = (CButtonWnd*)GetChildItem("AIFW_Step10Button");
		Step25Button = (CButtonWnd*)GetChildItem("AIFW_Step25Button");
		Step50Button = (CButtonWnd*)GetChildItem("AIFW_Step50Button");
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
		ReviewResistsLabel = GetChildItem("AIFW_ReviewResistsLabel");
		ReviewCombatLabel = GetChildItem("AIFW_ReviewCombatLabel");
		CreateButton = (CButtonWnd*)GetChildItem("AIFW_CreateButton");
		StatusLabel = GetChildItem("AIFW_StatusLabel");

		InitForgeFields();
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

			if (HandleStepClick(pWnd)) {
				UpdateView();
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

			if (HandleForgeFieldClick(pWnd)) {
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
		const std::string current_name = ReadName();
		const std::string previous_default = DefaultItemName();

		TypeIndex = ClampInt(index, 0, 4);
		if (current_name.empty() || current_name == previous_default) {
			SetEditText(DefaultItemName().c_str());
		}
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
		SetButtonText(Step1Button, StepIndex == 0 ? "> 1 <" : "1");
		SetButtonText(Step5Button, StepIndex == 1 ? "> 5 <" : "5");
		SetButtonText(Step10Button, StepIndex == 2 ? "> 10 <" : "10");
		SetButtonText(Step25Button, StepIndex == 3 ? "> 25 <" : "25");
		SetButtonText(Step50Button, StepIndex == 4 ? "> 50 <" : "50");
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

		UpdateForgeFieldLabels();
		char text[256];
		sprintf_s(text, "Use -/+ to tune every core stat, resist, heroic, and mod field.");
		SetLabel(StatsHintLabel, text);
		SetLabel(CombatHintLabel, UsesCombatValues() ? "Weapon values apply to blades. Other forms keep these at zero." : "This item form ignores weapon damage, delay, and haste.");

		const std::string name = ReadName();
		SetLabel(ReviewNameLabel, name.c_str());
		SetLabel(ReviewTypeLabel, BuildReviewHeader().c_str());
		SetLabel(ReviewStatsLabel, BuildReviewStats().c_str());
		SetLabel(ReviewResistsLabel, BuildReviewResists().c_str());
		SetLabel(ReviewCombatLabel, BuildReviewMods().c_str());
	}

	void CreateItem()
	{
		const std::string encoded_name = EncodeName(ReadName());
		std::string command = std::string("/say #itemforge draft type=") + TypeKey() + " name=" + encoded_name;
		NativeAutoLootSendCommand(command.c_str());

		for (const auto& field : ForgeFields) {
			if (!UsesCombatValues() && field.combat) {
				continue;
			}

			if (field.value == 0) {
				continue;
			}

			command = "/say #itemforge set ";
			command += field.key;
			command += "=";
			command += std::to_string(field.value);
			NativeAutoLootSendCommand(command.c_str());
		}

		NativeAutoLootSendCommand("/say #itemforge finish");
		SetStatus("Creating item. Watch chat and your cursor.");
	}

	struct ForgeField {
		const char* key;
		const char* label;
		const char* control_id;
		int value;
		int minimum;
		int maximum;
		int step;
		bool combat;
		CButtonWnd* minus;
		CXWnd* value_label;
		CButtonWnd* plus;
	};

	void InitForgeFields()
	{
		const ForgeField fields[ForgeFieldCount] = {
			{ "hp", "HP", "HP", 50, 0, 500, 25, false, nullptr, nullptr, nullptr },
			{ "mana", "Mana", "Mana", 25, 0, 500, 25, false, nullptr, nullptr, nullptr },
			{ "endur", "Endurance", "Endur", 0, 0, 500, 25, false, nullptr, nullptr, nullptr },
			{ "ac", "AC", "AC", 5, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "str", "STR", "Str", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "sta", "STA/CON", "Sta", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "dex", "DEX", "Dex", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "agi", "AGI", "Agi", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "int", "INT", "Int", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "wis", "WIS", "Wis", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "cha", "CHA", "Cha", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "mr", "Magic Resist", "MR", 0, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "fr", "Fire Resist", "FR", 0, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "cr", "Cold Resist", "CR", 0, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "pr", "Poison Resist", "PR", 0, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "dr", "Disease Resist", "DR", 0, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "svcorruption", "Corruption Resist", "Corrup", 0, 0, 100, 5, false, nullptr, nullptr, nullptr },
			{ "heroic_str", "Heroic STR", "HStr", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_sta", "Heroic STA", "HSta", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_dex", "Heroic DEX", "HDex", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_agi", "Heroic AGI", "HAgi", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_int", "Heroic INT", "HInt", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_wis", "Heroic WIS", "HWis", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_cha", "Heroic CHA", "HCha", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_mr", "Heroic MR", "HMR", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_fr", "Heroic FR", "HFR", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_cr", "Heroic CR", "HCR", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_pr", "Heroic PR", "HPR", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_dr", "Heroic DR", "HDR", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "heroic_svcorrup", "Heroic Corrup", "HCorrup", 0, 0, 25, 1, false, nullptr, nullptr, nullptr },
			{ "damage", "Damage", "Damage", 8, 0, 100, 2, true, nullptr, nullptr, nullptr },
			{ "delay", "Delay", "Delay", 24, 10, 60, 2, true, nullptr, nullptr, nullptr },
			{ "haste", "Haste", "Haste", 0, 0, 50, 5, true, nullptr, nullptr, nullptr },
			{ "attack", "Attack", "Attack", 0, 0, 250, 5, false, nullptr, nullptr, nullptr },
			{ "accuracy", "Accuracy", "Accuracy", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "avoidance", "Avoidance", "Avoidance", 0, 0, 100, 1, false, nullptr, nullptr, nullptr },
			{ "regen", "HP Regen", "Regen", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "manaregen", "Mana Regen", "ManaRegen", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "enduranceregen", "End Regen", "EndRegen", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "shielding", "Shielding", "Shielding", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "spellshield", "Spell Shield", "SpellShield", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "dotshielding", "DoT Shield", "DotShield", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "stunresist", "Stun Resist", "StunResist", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "strikethrough", "Strikethrough", "Strikethrough", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "damageshield", "Damage Shield", "DamageShield", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "dsmitigation", "DS Mitigation", "DSMitigation", 0, 0, 50, 1, false, nullptr, nullptr, nullptr },
			{ "healamt", "Heal Amount", "HealAmt", 0, 0, 250, 5, false, nullptr, nullptr, nullptr },
			{ "spelldmg", "Spell Damage", "SpellDmg", 0, 0, 250, 5, false, nullptr, nullptr, nullptr },
			{ "clairvoyance", "Clairvoyance", "Clairvoyance", 0, 0, 250, 5, false, nullptr, nullptr, nullptr },
			{ "backstabdmg", "Backstab Dmg", "BackstabDmg", 0, 0, 250, 5, false, nullptr, nullptr, nullptr }
		};

		for (int i = 0; i < ForgeFieldCount; ++i) {
			ForgeFields[i] = fields[i];
			char id[80];
			sprintf_s(id, "AIFW_%sMinusButton", ForgeFields[i].control_id);
			ForgeFields[i].minus = (CButtonWnd*)GetChildItem(id);
			sprintf_s(id, "AIFW_%sValueLabel", ForgeFields[i].control_id);
			ForgeFields[i].value_label = GetChildItem(id);
			sprintf_s(id, "AIFW_%sPlusButton", ForgeFields[i].control_id);
			ForgeFields[i].plus = (CButtonWnd*)GetChildItem(id);
		}
	}

	bool HandleForgeFieldClick(CXWnd* pWnd)
	{
		for (auto& field : ForgeFields) {
			if (pWnd == (CXWnd*)field.minus) {
				field.value = ClampInt(field.value - CurrentStep(), field.minimum, field.maximum);
				return true;
			}

			if (pWnd == (CXWnd*)field.plus) {
				field.value = ClampInt(field.value + CurrentStep(), field.minimum, field.maximum);
				return true;
			}
		}

		return false;
	}

	bool HandleStepClick(CXWnd* pWnd)
	{
		if (pWnd == (CXWnd*)Step1Button) { StepIndex = 0; return true; }
		if (pWnd == (CXWnd*)Step5Button) { StepIndex = 1; return true; }
		if (pWnd == (CXWnd*)Step10Button) { StepIndex = 2; return true; }
		if (pWnd == (CXWnd*)Step25Button) { StepIndex = 3; return true; }
		if (pWnd == (CXWnd*)Step50Button) { StepIndex = 4; return true; }
		return false;
	}

	int CurrentStep() const
	{
		static const int steps[5] = { 1, 5, 10, 25, 50 };
		return steps[ClampInt(StepIndex, 0, 4)];
	}

	void UpdateForgeFieldLabels()
	{
		char text[32];
		for (const auto& field : ForgeFields) {
			const int value = (!UsesCombatValues() && field.combat) ? 0 : field.value;
			sprintf_s(text, "%d", value);
			SetLabel(field.value_label, text);
		}
	}

	int FieldValue(const char* key) const
	{
		for (const auto& field : ForgeFields) {
			if (!strcmp(field.key, key)) {
				if (!UsesCombatValues() && field.combat) {
					return 0;
				}

				return field.value;
			}
		}

		return 0;
	}

	const char* SlotLine() const
	{
		switch (TypeIndex) {
		case 1:
			return "Chest";
		case 2:
			return "Finger";
		case 3:
			return "Charm";
		case 4:
			return "Secondary";
		default:
			return "Primary Secondary";
		}
	}

	std::string BuildReviewHeader() const
	{
		char text[256];
		sprintf_s(
			text,
			"Magic, No Trade, Quest\nClass: ALL\nRace: ALL\n%s",
			SlotLine()
		);
		return text;
	}

	void AppendReviewLine(std::string& text, const char* left_label, int left_value, const char* right_label = nullptr, int right_value = 0) const
	{
		if (left_value == 0 && (!right_label || right_value == 0)) {
			return;
		}

		char line[128];
		if (right_label && right_value != 0) {
			sprintf_s(line, "%-18s %4d    %-18s %4d\n", left_label, left_value, right_label, right_value);
		}
		else {
			sprintf_s(line, "%-18s %4d\n", left_label, left_value);
		}

		text += line;
	}

	std::string BuildReviewStats() const
	{
		char base[256];
		sprintf_s(
			base,
			"Size: LARGE          AC: %d\nWeight: 0.8          HP: %d\n                     Mana: %d\n                     End: %d\n\n",
			FieldValue("ac"),
			FieldValue("hp"),
			FieldValue("mana"),
			FieldValue("endur")
		);

		std::string text = base;
		AppendReviewLine(text, "Strength:", FieldValue("str"));
		AppendReviewLine(text, "Stamina:", FieldValue("sta"));
		AppendReviewLine(text, "Dexterity:", FieldValue("dex"));
		AppendReviewLine(text, "Agility:", FieldValue("agi"));
		AppendReviewLine(text, "Intelligence:", FieldValue("int"));
		AppendReviewLine(text, "Wisdom:", FieldValue("wis"));
		AppendReviewLine(text, "Charisma:", FieldValue("cha"));
		AppendReviewLine(text, "Heroic STR:", FieldValue("heroic_str"));
		AppendReviewLine(text, "Heroic STA:", FieldValue("heroic_sta"));
		AppendReviewLine(text, "Heroic DEX:", FieldValue("heroic_dex"));
		AppendReviewLine(text, "Heroic AGI:", FieldValue("heroic_agi"));
		AppendReviewLine(text, "Heroic INT:", FieldValue("heroic_int"));
		AppendReviewLine(text, "Heroic WIS:", FieldValue("heroic_wis"));
		AppendReviewLine(text, "Heroic CHA:", FieldValue("heroic_cha"));
		return text;
	}

	std::string BuildReviewResists() const
	{
		std::string text;
		AppendReviewLine(text, "Magic Resist:", FieldValue("mr"));
		AppendReviewLine(text, "Fire Resist:", FieldValue("fr"));
		AppendReviewLine(text, "Cold Resist:", FieldValue("cr"));
		AppendReviewLine(text, "Poison Resist:", FieldValue("pr"));
		AppendReviewLine(text, "Disease Resist:", FieldValue("dr"));
		AppendReviewLine(text, "Corruption:", FieldValue("svcorruption"));
		AppendReviewLine(text, "Heroic MR:", FieldValue("heroic_mr"));
		AppendReviewLine(text, "Heroic FR:", FieldValue("heroic_fr"));
		AppendReviewLine(text, "Heroic CR:", FieldValue("heroic_cr"));
		AppendReviewLine(text, "Heroic PR:", FieldValue("heroic_pr"));
		AppendReviewLine(text, "Heroic DR:", FieldValue("heroic_dr"));
		AppendReviewLine(text, "Heroic Corrup:", FieldValue("heroic_svcorrup"));
		return text.empty() ? "No resists selected." : text;
	}

	std::string BuildReviewMods() const
	{
		std::string text;
		if (UsesCombatValues()) {
			char weapon[128];
			sprintf_s(weapon, "Base Dmg: %d        Delay: %d\n", FieldValue("damage"), FieldValue("delay"));
			text += weapon;
			AppendReviewLine(text, "Haste:", FieldValue("haste"));
		}

		AppendReviewLine(text, "Attack:", FieldValue("attack"), "Accuracy:", FieldValue("accuracy"));
		AppendReviewLine(text, "Avoidance:", FieldValue("avoidance"), "HP Regen:", FieldValue("regen"));
		AppendReviewLine(text, "Mana Regen:", FieldValue("manaregen"), "End Regen:", FieldValue("enduranceregen"));
		AppendReviewLine(text, "Shielding:", FieldValue("shielding"), "Spell Shield:", FieldValue("spellshield"));
		AppendReviewLine(text, "DoT Shield:", FieldValue("dotshielding"), "Stun Resist:", FieldValue("stunresist"));
		AppendReviewLine(text, "Strikethrough:", FieldValue("strikethrough"), "Damage Shield:", FieldValue("damageshield"));
		AppendReviewLine(text, "DS Mitigation:", FieldValue("dsmitigation"), "Heal Amount:", FieldValue("healamt"));
		AppendReviewLine(text, "Spell Damage:", FieldValue("spelldmg"), "Clairvoyance:", FieldValue("clairvoyance"));
		AppendReviewLine(text, "Backstab Dmg:", FieldValue("backstabdmg"));

		if (text.empty()) {
			return "No combat or mod values selected.";
		}

		return text;
	}

	static const int ForgeFieldCount = 50;
	ForgeField ForgeFields[ForgeFieldCount];
	ItemPage CurrentPage = PageForm;
	int TypeIndex = 0;
	int StepIndex = 1;
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
	CButtonWnd* Step1Button = nullptr;
	CButtonWnd* Step5Button = nullptr;
	CButtonWnd* Step10Button = nullptr;
	CButtonWnd* Step25Button = nullptr;
	CButtonWnd* Step50Button = nullptr;
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
	CXWnd* ReviewResistsLabel = nullptr;
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

class NativeUIShowcaseWnd : public CCustomWnd
{
public:
	NativeUIShowcaseWnd() : CCustomWnd((char*)"NativeUIShowcaseWnd")
	{
		NativeAutoLootTrace("UI showcase constructor body entered");
		CloseOnESC = 1;
		SetWndNotification(NativeUIShowcaseWnd);

		InputsTab = (CButtonWnd*)GetChildItem("NUIS_InputsTab");
		DataTab = (CButtonWnd*)GetChildItem("NUIS_DataTab");
		TextTab = (CButtonWnd*)GetChildItem("NUIS_TextTab");
		InputsPage = GetChildItem("NUIS_InputsPage");
		DataPage = GetChildItem("NUIS_DataPage");
		TextPage = GetChildItem("NUIS_TextPage");

		NameEdit = (CEditWnd*)GetChildItem("NUIS_NameEdit");
		ModeCombo = (CComboWnd*)GetChildItem("NUIS_ModeCombo");
		PreviewCheck = (CButtonWnd*)GetChildItem("NUIS_EnablePreviewCheck");
		AutoApplyCheck = (CButtonWnd*)GetChildItem("NUIS_AutoApplyCheck");
		GoldRadio = (CButtonWnd*)GetChildItem("NUIS_GoldRadio");
		GreenRadio = (CButtonWnd*)GetChildItem("NUIS_GreenRadio");
		RedRadio = (CButtonWnd*)GetChildItem("NUIS_RedRadio");
		DensitySlider = (CSliderWnd*)GetChildItem("NUIS_DensitySlider");
		DensityMinusButton = (CButtonWnd*)GetChildItem("NUIS_DensityMinusButton");
		DensityPlusButton = (CButtonWnd*)GetChildItem("NUIS_DensityPlusButton");
		DensityValueLabel = GetChildItem("NUIS_DensityValueLabel");
		ApplyButton = (CButtonWnd*)GetChildItem("NUIS_ApplyButton");
		ResetButton = (CButtonWnd*)GetChildItem("NUIS_ResetButton");
		InputPreviewLabel = GetChildItem("NUIS_InputPreviewLabel");

		ControlList = (CListWnd*)GetChildItem("NUIS_ControlList");
		SelectFirstButton = (CButtonWnd*)GetChildItem("NUIS_SelectFirstButton");
		AddRowButton = (CButtonWnd*)GetChildItem("NUIS_AddRowButton");
		DataStatusLabel = GetChildItem("NUIS_DataStatusLabel");

		StoryText = (CStmlWnd*)GetChildItem("NUIS_StoryText");
		STMLPlainButton = (CButtonWnd*)GetChildItem("NUIS_STMLPlainButton");
		STMLColorButton = (CButtonWnd*)GetChildItem("NUIS_STMLColorButton");
		STMLAppendButton = (CButtonWnd*)GetChildItem("NUIS_STMLAppendButton");
		StatusLabel = GetChildItem("NUIS_StatusLabel");
		NativeAutoLootTrace("UI showcase child controls resolved");

		PopulateCombo();
		PopulateControlList();
		ResetDemo();
		ShowPage(PageInputs);
		SetSTMLPlain();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		if (pWnd == (CXWnd*)DensitySlider && (Message == XWM_NEWVALUE || Message == XWM_LMOUSEUP || Message == XWM_LCLICK)) {
			if (DensitySlider) {
				Density = ClampInt(DensitySlider->GetValue(), 0, 10);
				UpdateView();
				SetStatus("Slider changed.");
			}
			return 1;
		}

		if (pWnd == (CXWnd*)ModeCombo && (Message == XWM_NEWVALUE || Message == XWM_LCLICK)) {
			if (ModeCombo) {
				const int choice = ModeCombo->GetCurChoice();
				if (choice >= 0 && choice <= 3) {
					ModeIndex = choice;
					UpdateView();
					SetStatus("Dropdown selection changed.");
				}
			}
			return 1;
		}

		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)InputsTab) {
				ShowPage(PageInputs);
				return 1;
			}

			if (pWnd == (CXWnd*)DataTab) {
				ShowPage(PageData);
				return 1;
			}

			if (pWnd == (CXWnd*)TextTab) {
				ShowPage(PageText);
				return 1;
			}

			if (pWnd == (CXWnd*)PreviewCheck) {
				PreviewEnabled = !PreviewEnabled;
				UpdateView();
				SetStatus("Checkbox toggled.");
				return 1;
			}

			if (pWnd == (CXWnd*)AutoApplyCheck) {
				AutoApply = !AutoApply;
				UpdateView();
				SetStatus("Second checkbox toggled.");
				return 1;
			}

			if (pWnd == (CXWnd*)GoldRadio) {
				AccentIndex = 0;
				UpdateView();
				SetStatus("Radio group set to gold.");
				return 1;
			}

			if (pWnd == (CXWnd*)GreenRadio) {
				AccentIndex = 1;
				UpdateView();
				SetStatus("Radio group set to green.");
				return 1;
			}

			if (pWnd == (CXWnd*)RedRadio) {
				AccentIndex = 2;
				UpdateView();
				SetStatus("Radio group set to red.");
				return 1;
			}

			if (pWnd == (CXWnd*)DensityMinusButton) {
				Density = ClampInt(Density - 1, 0, 10);
				UpdateView();
				SetStatus("Stepper decreased the slider value.");
				return 1;
			}

			if (pWnd == (CXWnd*)DensityPlusButton) {
				Density = ClampInt(Density + 1, 0, 10);
				UpdateView();
				SetStatus("Stepper increased the slider value.");
				return 1;
			}

			if (pWnd == (CXWnd*)ApplyButton) {
				UpdateView();
				SetStatus("Applied the current input state locally.");
				return 1;
			}

			if (pWnd == (CXWnd*)ResetButton) {
				ResetDemo();
				SetStatus("Showcase controls reset.");
				return 1;
			}

			if (pWnd == (CXWnd*)ControlList) {
				UpdateSelectedListStatus();
				return 1;
			}

			if (pWnd == (CXWnd*)SelectFirstButton) {
				if (ControlList) {
					ControlList->SetCurSel(0);
				}
				UpdateSelectedListStatus();
				return 1;
			}

			if (pWnd == (CXWnd*)AddRowButton) {
				AddSampleRow();
				return 1;
			}

			if (pWnd == (CXWnd*)STMLPlainButton) {
				SetSTMLPlain();
				SetStatus("STML text reset to the plain sample.");
				return 1;
			}

			if (pWnd == (CXWnd*)STMLColorButton) {
				SetSTMLColor();
				SetStatus("STML text swapped to the color sample.");
				return 1;
			}

			if (pWnd == (CXWnd*)STMLAppendButton) {
				AppendSTMLLine();
				SetStatus("Appended an STML log line.");
				return 1;
			}
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Open()
	{
		pXWnd()->Show(1, 1);
		SetStatus("Native UI Showcase opened. Try the tabs, dropdown, checks, list, slider, gauge, and STML pane.");
		UpdateView();
	}

	void Layout()
	{
	}

	void ResetDemo()
	{
		ModeIndex = 1;
		AccentIndex = 0;
		Density = 5;
		PreviewEnabled = true;
		AutoApply = false;
		SetEditText("Prototype Panel");
		UpdateView();
	}

	void SetStatus(const char* text)
	{
		SetLabel(StatusLabel, text);
	}

private:
	enum ShowcasePage {
		PageInputs = 0,
		PageData = 1,
		PageText = 2
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

	void SetVisible(CXWnd* wnd, bool visible)
	{
		if (wnd) {
			wnd->Show(visible ? 1 : 0, 1);
		}
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

	void SetButtonCheck(CButtonWnd* button, bool checked)
	{
		if (button) {
			button->Checked = checked ? 1 : 0;
			button->SetCheck(checked);
		}
	}

	void SetEditText(const char* text)
	{
		if (!NameEdit) {
			return;
		}

		char buffer[96] = { 0 };
		strcpy_s(buffer, text && text[0] ? text : "Prototype Panel");
		SetCXStr(&NameEdit->InputText, buffer);
		CXStr value(buffer);
		((CXWnd*)NameEdit)->SetWindowTextA(value);
	}

	std::string ReadName() const
	{
		char text[96] = { 0 };
		if (NameEdit && NameEdit->InputText) {
			GetCXStr(NameEdit->InputText, text, sizeof(text));
		}

		if (!text[0]) {
			return "Prototype Panel";
		}

		return text;
	}

	const char* ModeLabel() const
	{
		switch (ModeIndex) {
		case 0:
			return "Compact";
		case 2:
			return "Dense Operations";
		case 3:
			return "Read Only";
		default:
			return "Comfortable";
		}
	}

	const char* AccentLabel() const
	{
		switch (AccentIndex) {
		case 1:
			return "Green";
		case 2:
			return "Red";
		default:
			return "Gold";
		}
	}

	COLORREF AccentColor() const
	{
		switch (AccentIndex) {
		case 1:
			return 0xFF66FF66;
		case 2:
			return 0xFFFF8080;
		default:
			return 0xFFFFFF80;
		}
	}

	void PopulateCombo()
	{
		if (!ModeCombo) {
			return;
		}

		ModeCombo->DeleteAll();
		ModeCombo->InsertChoice((char*)"Compact");
		ModeCombo->InsertChoice((char*)"Comfortable");
		ModeCombo->InsertChoice((char*)"Dense Operations");
		ModeCombo->InsertChoice((char*)"Read Only");
		ModeCombo->SetChoice(ModeIndex);
	}

	void AddListRow(const char* control, const char* pattern, const char* notes, COLORREF color, uint32_t data)
	{
		if (!ControlList) {
			return;
		}

		CXStr control_text(control);
		const int row = ControlList->AddString(control_text, color, data, nullptr, nullptr);
		CXStr pattern_text(pattern);
		CXStr notes_text(notes);
		ControlList->SetItemText(row, 1, &pattern_text);
		ControlList->SetItemText(row, 2, &notes_text);
		ControlList->SetItemColor(row, 2, color);
	}

	void PopulateControlList()
	{
		if (!ControlList) {
			return;
		}

		ControlList->DeleteAll();
		AddListRow("Button", "command", "click actions", 0xFFFFFFFF, 1);
		AddListRow("Checkbox", "toggle", "local bool state", 0xFF66FF66, 2);
		AddListRow("Radio", "choice set", "one active option", 0xFFFFFF80, 3);
		AddListRow("Combobox", "dropdown", "single selection", 0xFF80C0FF, 4);
		AddListRow("Slider", "numeric", "drag or stepper", 0xFFFFC080, 5);
		AddListRow("Listbox", "table", "rows and columns", 0xFFFFFFFF, 6);
		AddListRow("Gauge", "EQType", "bound client value", 0xFFFFFF80, 7);
		AddListRow("STMLbox", "rich text", "colored help/logs", 0xFF66FF66, 8);
		ControlList->SetCurSel(0);
		UpdateSelectedListStatus();
	}

	void AddSampleRow()
	{
		++SampleRows;
		char control[64];
		sprintf_s(control, "Sample %d", SampleRows);
		AddListRow(control, "runtime", "added from C++", AccentColor(), 100 + SampleRows);
		if (ControlList) {
			ControlList->SetCurSel(7 + SampleRows);
		}
		SetLabel(DataStatusLabel, "Added a runtime row with hidden item data and accent coloring.");
	}

	void UpdateSelectedListStatus()
	{
		if (!ControlList) {
			return;
		}

		const int selected = ControlList->GetCurSel();
		if (selected < 0) {
			SetLabel(DataStatusLabel, "Select a row to see listbox selection handling.");
			return;
		}

		const uint32_t data = ControlList->GetItemData(selected);
		char text[160];
		sprintf_s(text, "Selected row %d with hidden data %u. List rows can carry text, colors, selection, and item data.", selected + 1, data);
		SetLabel(DataStatusLabel, text);
	}

	void ShowPage(ShowcasePage page)
	{
		CurrentPage = page;
		SetVisible(InputsPage, CurrentPage == PageInputs);
		SetVisible(DataPage, CurrentPage == PageData);
		SetVisible(TextPage, CurrentPage == PageText);
		UpdateTabs();
	}

	void UpdateTabs()
	{
		SetButtonText(InputsTab, CurrentPage == PageInputs ? "> Inputs <" : "Inputs");
		SetButtonText(DataTab, CurrentPage == PageData ? "> Data <" : "Data Views");
		SetButtonText(TextTab, CurrentPage == PageText ? "> Text <" : "Rich Text");
	}

	void UpdateView()
	{
		UpdateTabs();

		if (ModeCombo) {
			ModeCombo->SetChoice(ModeIndex);
		}

		if (DensitySlider) {
			DensitySlider->SetNumTicks(10);
			DensitySlider->SetValue(Density);
		}

		SetButtonCheck(PreviewCheck, PreviewEnabled);
		SetButtonCheck(AutoApplyCheck, AutoApply);
		SetButtonCheck(GoldRadio, AccentIndex == 0);
		SetButtonCheck(GreenRadio, AccentIndex == 1);
		SetButtonCheck(RedRadio, AccentIndex == 2);

		SetButtonText(PreviewCheck, PreviewEnabled ? "Preview: On" : "Preview: Off");
		SetButtonText(AutoApplyCheck, AutoApply ? "Auto Apply: On" : "Auto Apply: Off");
		SetButtonText(GoldRadio, AccentIndex == 0 ? "* Gold" : "Gold");
		SetButtonText(GreenRadio, AccentIndex == 1 ? "* Green" : "Green");
		SetButtonText(RedRadio, AccentIndex == 2 ? "* Red" : "Red");

		char value[32];
		sprintf_s(value, "%d", Density);
		SetLabel(DensityValueLabel, value);

		char preview[384];
		sprintf_s(
			preview,
			"Name: %s\nMode: %s\nPreview: %s  Auto Apply: %s  Accent: %s  Density: %d\nThis is a local-only playground for native EQ controls before we commit to a feature workflow.",
			ReadName().c_str(),
			ModeLabel(),
			PreviewEnabled ? "on" : "off",
			AutoApply ? "on" : "off",
			AccentLabel(),
			Density
		);
		SetLabel(InputPreviewLabel, preview);
	}

	void SetSTML(const char* text)
	{
		if (StoryText) {
			CXStr value(text ? text : "");
			StoryText->SetSTMLText(value, false, nullptr);
			StoryText->ForceParseNow();
		}
	}

	void SetSTMLPlain()
	{
		SetSTML(
			"Native UI STML sample<br>"
			"<br>"
			"Use this for help panes, compact summaries, changelogs, walkthroughs, or output logs.<br>"
			"Buttons below swap the content or append a new line without recreating the window."
		);
	}

	void SetSTMLColor()
	{
		SetSTML(
			"<c \"#66ff66\">Green:</c> success or enabled state<br>"
			"<c \"#ffff80\">Gold:</c> warning, pending, or important metadata<br>"
			"<c \"#ff8080\">Red:</c> blocked or destructive action<br>"
			"<br>"
			"STML is a useful middle ground between plain labels and a full list view."
		);
	}

	void AppendSTMLLine()
	{
		if (!StoryText) {
			return;
		}

		++LogLine;
		char line[128];
		sprintf_s(line, "<br>Appended runtime line %d with density %d and %s accent.", LogLine, Density, AccentLabel());
		CXStr value(line);
		StoryText->AppendSTML(value);
		StoryText->ForceParseNow();
	}

	ShowcasePage CurrentPage = PageInputs;
	int ModeIndex = 1;
	int AccentIndex = 0;
	int Density = 5;
	int SampleRows = 0;
	int LogLine = 0;
	bool PreviewEnabled = true;
	bool AutoApply = false;

	CButtonWnd* InputsTab = nullptr;
	CButtonWnd* DataTab = nullptr;
	CButtonWnd* TextTab = nullptr;
	CXWnd* InputsPage = nullptr;
	CXWnd* DataPage = nullptr;
	CXWnd* TextPage = nullptr;
	CEditWnd* NameEdit = nullptr;
	CComboWnd* ModeCombo = nullptr;
	CButtonWnd* PreviewCheck = nullptr;
	CButtonWnd* AutoApplyCheck = nullptr;
	CButtonWnd* GoldRadio = nullptr;
	CButtonWnd* GreenRadio = nullptr;
	CButtonWnd* RedRadio = nullptr;
	CSliderWnd* DensitySlider = nullptr;
	CButtonWnd* DensityMinusButton = nullptr;
	CButtonWnd* DensityPlusButton = nullptr;
	CXWnd* DensityValueLabel = nullptr;
	CButtonWnd* ApplyButton = nullptr;
	CButtonWnd* ResetButton = nullptr;
	CXWnd* InputPreviewLabel = nullptr;
	CListWnd* ControlList = nullptr;
	CButtonWnd* SelectFirstButton = nullptr;
	CButtonWnd* AddRowButton = nullptr;
	CXWnd* DataStatusLabel = nullptr;
	CStmlWnd* StoryText = nullptr;
	CButtonWnd* STMLPlainButton = nullptr;
	CButtonWnd* STMLColorButton = nullptr;
	CButtonWnd* STMLAppendButton = nullptr;
	CXWnd* StatusLabel = nullptr;
};

static NativeUIShowcaseWnd* gNativeUIShowcaseWnd = nullptr;

static bool NativeUIShowcaseFileExists(const char* path)
{
	FILE* file = nullptr;
	if (fopen_s(&file, path, "r") || !file) {
		return false;
	}

	fclose(file);
	return true;
}

static bool NativeUIShowcaseFileContains(const char* path, const char* needle)
{
	FILE* file = nullptr;
	if (fopen_s(&file, path, "r") || !file) {
		return false;
	}

	char line[1024];
	while (fgets(line, sizeof(line), file)) {
		if (strstr(line, needle)) {
			fclose(file);
			return true;
		}
	}

	fclose(file);
	return false;
}

struct NativeHpFixState
{
	bool has_payload = false;
	long long current = 0;
	long long maximum = 0;
	float percent = 0.0f;
};

static NativeHpFixState gNativeHpFixState;

static std::string NativeHpFixFormatInteger(long long value)
{
	char raw[64];
	sprintf_s(raw, "%lld", value);

	std::string text(raw);
	const bool negative = !text.empty() && text[0] == '-';
	int insert_at = static_cast<int>(text.size()) - 3;
	const int first_digit = negative ? 1 : 0;

	while (insert_at > first_digit) {
		text.insert(static_cast<size_t>(insert_at), ",");
		insert_at -= 3;
	}

	return text;
}

static std::string NativeHpFixFormatCompact(long long value)
{
	const bool negative = value < 0;
	const long long abs_value = negative ? -value : value;
	const char* suffix = "";
	double scaled = static_cast<double>(abs_value);

	if (abs_value >= 1000000000LL) {
		suffix = "B";
		scaled = static_cast<double>(abs_value) / 1000000000.0;
	}
	else if (abs_value >= 1000000LL) {
		suffix = "M";
		scaled = static_cast<double>(abs_value) / 1000000.0;
	}
	else if (abs_value >= 1000LL) {
		return NativeHpFixFormatInteger(value);
	}
	else {
		return NativeHpFixFormatInteger(value);
	}

	char text[64];
	sprintf_s(
		text,
		scaled >= 100.0 ? "%s%.0f%s" : "%s%.1f%s",
		negative ? "-" : "",
		scaled,
		suffix
	);
	return text;
}

static long long NativeHpFixClampHp(long long value)
{
	if (value < 0) {
		return 0;
	}

	if (value > 2147483647LL) {
		return 2147483647LL;
	}

	return value;
}

static DWORD NativeHpFixToDwordHp(long long value)
{
	return static_cast<DWORD>(NativeHpFixClampHp(value));
}

static LONG NativeHpFixToLongHp(long long value)
{
	return static_cast<LONG>(NativeHpFixClampHp(value));
}

static int NativeHpFixToClientIntHp(long long value)
{
	if (value < 0) {
		return 0;
	}

	if (value > 0x7fffffffLL) {
		return 0x7fffffff;
	}

	return static_cast<int>(value);
}

static void NativeHpFixSetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text ? text : "");
		label->SetWindowTextA(value);
	}
}

static CXWnd* NativeHpFixFindChild(CXWnd* parent, const char* child_name)
{
	if (!parent || !child_name || !child_name[0]) {
		return nullptr;
	}

	CXWnd* child = nullptr;
	__try {
		child = parent->GetChildItem((char*)child_name);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		child = nullptr;
	}

	if (!child) {
		return nullptr;
	}

	return child;
}

static void NativeHpFixSetChildLabel(CXWnd* parent, const char* child_name, const char* text, bool show)
{
	CXWnd* child = NativeHpFixFindChild(parent, child_name);
	if (!child) {
		return;
	}

	NativeHpFixSetLabel(child, text);
	if (show) {
		child->Show(1, 1);
	}
}

bool NativeHpFixGetEqTypeLabel(DWORD eq_type, const char* control_name, char* out, size_t out_size)
{
	if (!out || out_size == 0) {
		return false;
	}

	out[0] = '\0';

	if (!gNativeHpFixState.has_payload) {
		return false;
	}

	std::string text;
	switch (eq_type) {
		case 17: // Current HP labels
			text = (control_name && !strncmp(control_name, "IWS_", 4)) ?
				NativeHpFixFormatInteger(gNativeHpFixState.current) :
				NativeHpFixFormatCompact(gNativeHpFixState.current);
			break;
		case 18: // Maximum HP labels
			text = (control_name && !strncmp(control_name, "IWS_", 4)) ?
				NativeHpFixFormatInteger(gNativeHpFixState.maximum) :
				NativeHpFixFormatCompact(gNativeHpFixState.maximum);
			break;
		case 19: { // Player HP percent label
			int percent = static_cast<int>(gNativeHpFixState.percent + 0.5f);
			if (percent < 0) {
				percent = 0;
			}
			else if (percent > 100) {
				percent = 100;
			}

			char percent_text[32];
			sprintf_s(percent_text, "%d", percent);
			text = percent_text;
			break;
		}
		default:
			return false;
	}

	strncpy_s(out, out_size, text.c_str(), _TRUNCATE);
	return true;
}

bool NativeHpFixGetClientHpValues(int* current, int* maximum, int* percent_out)
{
	if (!gNativeHpFixState.has_payload) {
		return false;
	}

	if (current) {
		*current = NativeHpFixToClientIntHp(gNativeHpFixState.current);
	}

	if (maximum) {
		*maximum = NativeHpFixToClientIntHp(gNativeHpFixState.maximum);
	}

	if (percent_out) {
		int percent = static_cast<int>(gNativeHpFixState.percent + 0.5f);
		if (percent < 0) {
			percent = 0;
		}
		else if (percent > 100) {
			percent = 100;
		}

		*percent_out = percent;
	}

	return true;
}

bool NativeHpFixGetGaugeValue(DWORD eq_type, int* out_value)
{
	if (!out_value || !gNativeHpFixState.has_payload) {
		return false;
	}

	switch (eq_type) {
		case 1:  // Player HP gauge
			break;
		default:
			return false;
	}

	int percent = static_cast<int>(gNativeHpFixState.percent + 0.5f);
	if (percent < 0) {
		percent = 0;
	}
	else if (percent > 100) {
		percent = 100;
	}

	*out_value = percent;
	return true;
}

class NativeHpFixWnd : public CCustomWnd
{
public:
	NativeHpFixWnd() : CCustomWnd((char*)"NativeHpFixWnd")
	{
		CloseOnESC = 1;
		SetWndNotification(NativeHpFixWnd);

		StatusLabel = GetChildItem("HPFIX_StatusLabel");
		CurrentLabel = GetChildItem("HPFIX_CurrentLabel");
		MaxLabel = GetChildItem("HPFIX_MaxLabel");
		PercentLabel = GetChildItem("HPFIX_PercentLabel");
		DetailLabel = GetChildItem("HPFIX_DetailLabel");

		SetStatus("Waiting for native HPFIX payload...");
		Refresh();
	}

	int WndNotification(CXWnd* pWnd, unsigned int Message, void* unknown)
	{
		if (Message == XWM_CLOSE) {
			pXWnd()->Show(0, 1);
			return 1;
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	}

	void Open()
	{
		pXWnd()->Show(1, 1);
		Refresh();
	}

	void Refresh()
	{
		if (!gNativeHpFixState.has_payload) {
			NativeHpFixSetLabel(CurrentLabel, "Current: pending");
			NativeHpFixSetLabel(MaxLabel, "Maximum: pending");
			NativeHpFixSetLabel(PercentLabel, "Percent: pending");
			NativeHpFixSetLabel(DetailLabel, "The server will send authoritative self HP after the DLL handshake.");
			return;
		}

		const std::string current = "Current: " + NativeHpFixFormatInteger(gNativeHpFixState.current);
		const std::string maximum = "Maximum: " + NativeHpFixFormatInteger(gNativeHpFixState.maximum);

		char percent[64];
		sprintf_s(percent, "Percent: %.2f%%", gNativeHpFixState.percent);

		NativeHpFixSetLabel(CurrentLabel, current.c_str());
		NativeHpFixSetLabel(MaxLabel, maximum.c_str());
		NativeHpFixSetLabel(PercentLabel, percent);
		NativeHpFixSetLabel(DetailLabel, "Normal player and inventory windows are patched from server-authoritative HP.");
	}

	void SetStatus(const char* text)
	{
		NativeHpFixSetLabel(StatusLabel, text);
	}

private:
	CXWnd* StatusLabel = nullptr;
	CXWnd* CurrentLabel = nullptr;
	CXWnd* MaxLabel = nullptr;
	CXWnd* PercentLabel = nullptr;
	CXWnd* DetailLabel = nullptr;
};

static NativeHpFixWnd* gNativeHpFixWnd = nullptr;

static bool NativeHpFixClientFilesReady()
{
	char xml_path[MAX_PATH];
	char equi_path[MAX_PATH];
	if (gszEQPath[0]) {
		sprintf_s(xml_path, "%s\\uifiles\\default\\EQUI_NativeHpFixWnd.xml", gszEQPath);
		sprintf_s(equi_path, "%s\\uifiles\\default\\EQUI.xml", gszEQPath);
	}
	else {
		strcpy_s(xml_path, "uifiles\\default\\EQUI_NativeHpFixWnd.xml");
		strcpy_s(equi_path, "uifiles\\default\\EQUI.xml");
	}

	if (!NativeUIShowcaseFileExists(xml_path)) {
		NativeAutoLootTrace("HPFIX XML missing: %s", xml_path);
		return false;
	}

	if (!NativeUIShowcaseFileContains(equi_path, "EQUI_NativeHpFixWnd.xml")) {
		NativeAutoLootTrace("HPFIX XML is not included by EQUI.xml: %s", equi_path);
		return false;
	}

	return true;
}

static void NativeHpFixEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeHpFixWnd) {
		if (!NativeHpFixClientFilesReady()) {
			return;
		}

		NativeAutoLootTrace("creating HPFIX window");
		NativeHpFixWnd* created_window = nullptr;
		__try {
			created_window = new NativeHpFixWnd();
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("HPFIX window construction faulted; check EQUI_NativeHpFixWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			return;
		}

		gNativeHpFixWnd = created_window;
		NativeAutoLootTrace("HPFIX window created");
	}

	if (show && gNativeHpFixWnd) {
		gNativeHpFixWnd->Open();
	}
}

static void NativeHpFixPatchSpawnData(PSPAWNINFO spawn)
{
	if (!spawn || !gNativeHpFixState.has_payload) {
		return;
	}

	spawn->HPMax = NativeHpFixToDwordHp(gNativeHpFixState.maximum);
	spawn->HPCurrent = NativeHpFixToLongHp(gNativeHpFixState.current);
}

static void NativeHpFixPatchPlayerData(EQPlayer* player)
{
	if (!player || !gNativeHpFixState.has_payload) {
		return;
	}

	player->Data.HPMax = NativeHpFixToDwordHp(gNativeHpFixState.maximum);
	player->Data.HPCurrent = NativeHpFixToLongHp(gNativeHpFixState.current);
}

static void NativeHpFixPatchLocalState()
{
	if (!gNativeHpFixState.has_payload) {
		return;
	}

	__try {
		if (ppLocalPlayer && pLocalPlayer) {
			NativeHpFixPatchPlayerData(pLocalPlayer);
		}

		if (ppCharSpawn && pCharSpawn && (!ppLocalPlayer || pCharSpawn != pLocalPlayer)) {
			NativeHpFixPatchPlayerData(pCharSpawn);
		}

		PCHARINFO char_info = GetCharInfo();
		if (char_info && char_info->pSpawn) {
			NativeHpFixPatchSpawnData(char_info->pSpawn);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("HPFIX could not patch local spawn HP state");
	}
}

static void NativeHpFixMaintainNormalUi()
{
	if (!gNativeHpFixState.has_payload) {
		return;
	}

	NativeHpFixPatchLocalState();

	int percent = static_cast<int>(gNativeHpFixState.percent + 0.5f);
	if (percent < 0) {
		percent = 0;
	}
	else if (percent > 100) {
		percent = 100;
	}

	char percent_text[32];
	sprintf_s(percent_text, "%d", percent);

	const std::string current_exact = NativeHpFixFormatInteger(gNativeHpFixState.current);
	const std::string maximum_exact = NativeHpFixFormatInteger(gNativeHpFixState.maximum);
	const std::string current_compact = NativeHpFixFormatCompact(gNativeHpFixState.current);
	const std::string maximum_compact = NativeHpFixFormatCompact(gNativeHpFixState.maximum);

	if (ppPlayerWnd && pPlayerWnd) {
		NativeHpFixSetChildLabel((CXWnd*)pPlayerWnd, "Player_HPLabel", percent_text, true);
		NativeHpFixSetChildLabel((CXWnd*)pPlayerWnd, "HPLabel", percent_text, true);
		NativeHpFixSetChildLabel((CXWnd*)pPlayerWnd, "Player_HPPercLabel", "%", true);
		NativeHpFixSetChildLabel((CXWnd*)pPlayerWnd, "HPPerLabel", "%", true);
	}

	if (ppInventoryWnd && pInventoryWnd) {
		NativeHpFixSetChildLabel((CXWnd*)pInventoryWnd, "IW_CurrentHP", current_compact.c_str(), true);
		NativeHpFixSetChildLabel((CXWnd*)pInventoryWnd, "HPNumberLabel", current_compact.c_str(), true);
		NativeHpFixSetChildLabel((CXWnd*)pInventoryWnd, "IW_MaxHP", maximum_compact.c_str(), true);
		NativeHpFixSetChildLabel((CXWnd*)pInventoryWnd, "IWS_CurrentHP", current_exact.c_str(), true);
		NativeHpFixSetChildLabel((CXWnd*)pInventoryWnd, "IWS_MaxHP", maximum_exact.c_str(), true);
	}
}

static bool NativeHpFixApplyPayload(const std::string& payload)
{
	const long long current = NativeToInt64(NativeGetPairValue(payload, "current"), -1);
	const long long maximum = NativeToInt64(NativeGetPairValue(payload, "max"), -1);
	if (current < 0 || maximum <= 0) {
		NativeAutoLootTrace("HPFIX ignored invalid payload: %s", payload.c_str());
		return true;
	}

	gNativeHpFixState.has_payload = true;
	gNativeHpFixState.current = current;
	gNativeHpFixState.maximum = maximum;
	gNativeHpFixState.percent = NativeToFloat(
		NativeGetPairValue(payload, "percent"),
		maximum > 0 ? static_cast<float>((static_cast<double>(current) * 100.0) / static_cast<double>(maximum)) : 0.0f
	);
	gNativeHpFixSentReady = true;
	gNativeHpFixReadyRetryPulses = 0;
	gNativeHpFixRefreshPulses = 0;

	NativeHpFixMaintainNormalUi();
	if (gNativeHpFixWnd) {
		gNativeHpFixWnd->SetStatus("Authoritative HP received.");
		gNativeHpFixWnd->Refresh();
	}

	NativeAutoLootTrace(
		"HPFIX update current=%lld maximum=%lld percent=%.2f",
		gNativeHpFixState.current,
		gNativeHpFixState.maximum,
		gNativeHpFixState.percent
	);
	return true;
}

static void NativeHpFixPulseSync()
{
	if (!gNativeHpFixState.has_payload) {
		gNativeHpFixRefreshPulses = 0;
		if (++gNativeHpFixReadyRetryPulses >= 30) {
			gNativeHpFixReadyRetryPulses = 0;
			gNativeHpFixSentReady = true;
			NativeAutoLootSendCommand("/say #hpfix native ready");
		}
		return;
	}

	gNativeHpFixReadyRetryPulses = 0;
	if (++gNativeHpFixRefreshPulses >= 60) {
		gNativeHpFixRefreshPulses = 0;
		NativeAutoLootSendCommand("/say #hpfix native refresh");
	}
}

static bool NativeUIShowcaseClientFilesReady()
{
	char xml_path[MAX_PATH];
	char equi_path[MAX_PATH];
	if (gszEQPath[0]) {
		sprintf_s(xml_path, "%s\\uifiles\\default\\EQUI_NativeUIShowcaseWnd.xml", gszEQPath);
		sprintf_s(equi_path, "%s\\uifiles\\default\\EQUI.xml", gszEQPath);
	}
	else {
		strcpy_s(xml_path, "uifiles\\default\\EQUI_NativeUIShowcaseWnd.xml");
		strcpy_s(equi_path, "uifiles\\default\\EQUI.xml");
	}

	if (!NativeUIShowcaseFileExists(xml_path)) {
		NativeAutoLootTrace("UI showcase XML missing: %s", xml_path);
		return false;
	}

	if (!NativeUIShowcaseFileContains(equi_path, "EQUI_NativeUIShowcaseWnd.xml")) {
		NativeAutoLootTrace("UI showcase XML is not included by EQUI.xml: %s", equi_path);
		return false;
	}

	return true;
}

static void NativeUIShowcaseEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeUIShowcaseWnd) {
		if (!NativeUIShowcaseClientFilesReady()) {
			NativeAutoLootTrace("UI showcase window not created because client files are incomplete");
			return;
		}

		NativeAutoLootTrace("creating UI showcase window");
		NativeUIShowcaseWnd* created_window = nullptr;
		__try {
			created_window = new NativeUIShowcaseWnd();
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("UI showcase window construction faulted; check EQUI_NativeUIShowcaseWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			return;
		}

		gNativeUIShowcaseWnd = created_window;
		NativeAutoLootTrace("UI showcase window created");
	}

	if (show && gNativeUIShowcaseWnd) {
		gNativeUIShowcaseWnd->Open();
	}
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

void NativeAchievementWnd::RefreshRewardList()
{
	if (!RewardList) {
		return;
	}

	RewardList->DeleteAll();
	if (gNativeAchievementRewards.empty()) {
		CXStr dash("-");
		const int row = RewardList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No rewards listed.");
		RewardList->SetItemText(row, 1, &empty);
		gNativeAchievementRewardsDirty = false;
		return;
	}

	for (const NativeAchievementRewardRow& reward : gNativeAchievementRewards) {
		CXStr state(reward.auto_claim ? "Auto" : "Claim");
		const int row = RewardList->AddString(state, reward.auto_claim ? 0xFF66FF66 : 0xFFFFFF80, (uint32_t)reward.definition_id, nullptr, nullptr);

		std::string reward_text = reward.name.empty() ? reward.type : reward.name;
		if (!reward.tier.empty()) {
			reward_text += " [";
			reward_text += reward.tier;
			reward_text += "]";
		}

		CXStr name(reward_text.c_str());
		RewardList->SetItemText(row, 1, &name);
	}

	gNativeAchievementRewardsDirty = false;
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

	if (gNativeAchievementRewardsDirty) {
		RefreshRewardList();
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

std::string NativeFactionWnd::ReadSearch(bool prefer_input_text) const
{
	char input_text[96] = { 0 };
	char window_text_buffer[96] = { 0 };
	bool has_input_text = false;
	bool has_window_text = false;
	if (SearchEdit) {
		if (SearchEdit->InputText) {
			has_input_text = true;
			GetCXStr(SearchEdit->InputText, input_text, sizeof(input_text));
		}

		__try {
			CXStr window_text = ((CXWnd*)SearchEdit)->GetWindowTextA();
			if (window_text.Ptr) {
				has_window_text = true;
				GetCXStr(window_text.Ptr, window_text_buffer, sizeof(window_text_buffer));
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			has_window_text = false;
			window_text_buffer[0] = 0;
		}
	}

	const char* text = "";
	if (prefer_input_text && has_input_text) {
		text = input_text;
	}
	else if (has_window_text) {
		text = window_text_buffer;
	}
	else if (has_input_text) {
		text = input_text;
	}

	std::string search(text);
	while (!search.empty() && std::isspace(static_cast<unsigned char>(search.front()))) {
		search.erase(search.begin());
	}

	while (!search.empty() && std::isspace(static_cast<unsigned char>(search.back()))) {
		search.pop_back();
	}

	return search;
}

void NativeFactionWnd::SetSearchText(const char* text)
{
	if (!SearchEdit) {
		return;
	}

	char buffer[96] = { 0 };
	strncpy_s(buffer, sizeof(buffer), text ? text : "", _TRUNCATE);
	UpdatingSearchText = true;
	SetCXStr(&SearchEdit->InputText, buffer);
	CXStr value(buffer);
	((CXWnd*)SearchEdit)->SetWindowTextA(value);
	UpdatingSearchText = false;
}

void NativeFactionWnd::ApplySearchFromEdit(bool server_search, bool defer_refresh)
{
	const std::string search = ReadSearch(defer_refresh);
	const bool changed = search != gNativeFactionSearch;
	gNativeFactionSearch = search;

	if (changed) {
		gNativeFactionRowsDirty = true;
		if (defer_refresh) {
			DeferredSearchRefresh = true;
		}
		else {
			RefreshRows();
		}
	}

	if (!server_search) {
		return;
	}

	if (search.empty()) {
		NativeAutoLootSendCommand("/say #rep refresh");
		SetStatus("Refreshing faction reputation...");
		return;
	}

	const std::string command = std::string("/say #rep search ") + search;
	NativeAutoLootSendCommand(command.c_str());
	SetStatus("Searching faction reputation...");
}

void NativeFactionWnd::ClearSearch()
{
	gNativeFactionSearch.clear();
	DeferredSearchRefresh = false;
	SetSearchText("");
	gNativeFactionRowsDirty = true;
	RefreshRows();
	NativeAutoLootSendCommand("/say #rep refresh");
	SetStatus("Clearing faction search...");
}

void NativeFactionWnd::RefreshRows()
{
	const std::string search_lower = NativeLower(gNativeFactionSearch);
	unsigned visible_rows = 0;
	for (const NativeFactionRow& faction : gNativeFactionRows) {
		if (NativeFactionRowMatchesSearch(faction, search_lower)) {
			++visible_rows;
		}
	}

	char summary[192];
	sprintf_s(
		summary,
		"Faction rows %u/%u  Target %s%s%s%s%s",
		visible_rows,
		static_cast<unsigned>(gNativeFactionRows.size()),
		gNativeFactionTargetId > 0 ? "selected" : "none",
		gNativeFactionMode.empty() ? "" : "  Mode ",
		gNativeFactionMode.empty() ? "" : gNativeFactionMode.c_str(),
		gNativeFactionSearch.empty() ? "" : "  Search ",
		gNativeFactionSearch.empty() ? "" : gNativeFactionSearch.c_str()
	);
	SetLabel(SummaryLabel, summary);
	SetStatus(gNativeFactionStatus.c_str());

	if (!FactionList || !gNativeFactionRowsDirty) {
		return;
	}

	FactionList->DeleteAll();
	if (gNativeFactionRows.empty()) {
		CXStr dash("-");
		const int row = FactionList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No faction standings loaded.");
		FactionList->SetItemText(row, 1, &empty);
		FactionList->SetItemText(row, 2, &dash);
		FactionList->SetItemText(row, 3, &dash);
		FactionList->SetItemText(row, 4, &dash);
		FactionList->SetItemText(row, 5, &dash);
		gNativeFactionRowsDirty = false;
		return;
	}

	if (visible_rows == 0) {
		CXStr dash("-");
		const int row = FactionList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No matching faction standings.");
		FactionList->SetItemText(row, 1, &empty);
		FactionList->SetItemText(row, 2, &dash);
		FactionList->SetItemText(row, 3, &dash);
		FactionList->SetItemText(row, 4, &dash);
		FactionList->SetItemText(row, 5, &dash);
		gNativeFactionRowsDirty = false;
		return;
	}

	for (const NativeFactionRow& faction : gNativeFactionRows) {
		if (!NativeFactionRowMatchesSearch(faction, search_lower)) {
			continue;
		}

		const char* pin = faction.target ? "Target" : (faction.pinned ? "Pinned" : (faction.hidden ? "Hidden" : (faction.touched ? "Changed" : "-")));
		CXStr pin_text(pin);
		const int row = FactionList->AddString(pin_text, RowColor(faction.standing), static_cast<uint32_t>(faction.id), nullptr, nullptr);

		char raw_text[32];
		char modified_text[32];
		sprintf_s(raw_text, "%d", faction.raw_value);
		sprintf_s(modified_text, "%d", faction.modified_value);

		CXStr name(faction.name.c_str());
		CXStr standing(faction.standing.c_str());
		CXStr raw(raw_text);
		CXStr modified(modified_text);
		CXStr known(faction.section.empty() ? (faction.touched ? "Changed" : "All") : faction.section.c_str());
		FactionList->SetItemText(row, 1, &name);
		FactionList->SetItemText(row, 2, &standing);
		FactionList->SetItemText(row, 3, &raw);
		FactionList->SetItemText(row, 4, &modified);
		FactionList->SetItemText(row, 5, &known);
	}

	gNativeFactionRowsDirty = false;
}

void NativeDpsWnd::RefreshRows()
{
	char summary[192];
	sprintf_s(
		summary,
		"%s  %llu dmg / %llu heal / %llu inc  %.1fs",
		gNativeDpsTarget.empty() ? "No encounter" : gNativeDpsTarget.c_str(),
		gNativeDpsDamage,
		gNativeDpsHealing,
		gNativeDpsIncoming,
		gNativeDpsElapsedMs > 0 ? static_cast<double>(gNativeDpsElapsedMs) / 1000.0 : 0.0
	);
	SetLabel(SummaryLabel, summary);
	SetStatus(gNativeDpsStatus.c_str());

	if (!DpsList || !gNativeDpsRowsDirty) {
		return;
	}

	DpsList->DeleteAll();
	if (gNativeDpsRows.empty()) {
		CXStr dash("-");
		const int row = DpsList->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No DPS rows loaded.");
		DpsList->SetItemText(row, 1, &empty);
		for (int column = 2; column <= 7; ++column) {
			DpsList->SetItemText(row, column, &dash);
		}
		gNativeDpsRowsDirty = false;
		return;
	}

	for (const NativeDpsRow& dps : gNativeDpsRows) {
		CXStr actor(dps.actor.c_str());
		const int row = DpsList->AddString(actor, 0xFFFFFFFF, static_cast<uint32_t>(dps.actor_id), nullptr, nullptr);

		char damage[32];
		char dps_text[32];
		char healing[32];
		char hps_text[32];
		char incoming[32];
		char pct[32];
		sprintf_s(damage, "%llu", dps.damage);
		sprintf_s(dps_text, "%llu", dps.dps);
		sprintf_s(healing, "%llu", dps.healing);
		sprintf_s(hps_text, "%llu", dps.hps);
		sprintf_s(incoming, "%llu", dps.incoming);
		sprintf_s(pct, "%d%%", dps.pct);

		CXStr source(dps.source.c_str());
		CXStr damage_str(damage);
		CXStr dps_str(dps_text);
		CXStr healing_str(healing);
		CXStr hps_str(hps_text);
		CXStr incoming_str(incoming);
		CXStr pct_str(pct);
		DpsList->SetItemText(row, 1, &source);
		DpsList->SetItemText(row, 2, &damage_str);
		DpsList->SetItemText(row, 3, &dps_str);
		DpsList->SetItemText(row, 4, &healing_str);
		DpsList->SetItemText(row, 5, &hps_str);
		DpsList->SetItemText(row, 6, &incoming_str);
		DpsList->SetItemText(row, 7, &pct_str);
	}

	gNativeDpsRowsDirty = false;
}

static void NativeFactionEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeFactionWnd) {
		NativeAutoLootTrace("creating faction reputation window");
		gNativeFactionWnd = new NativeFactionWnd();
		gNativeFactionWnd->RefreshRows();
		NativeAutoLootTrace("faction reputation window created");
	}

	if (show && gNativeFactionWnd) {
		gNativeFactionWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeDpsEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeDpsWnd) {
		NativeAutoLootTrace("creating DPS parser window");
		gNativeDpsWnd = new NativeDpsWnd();
		gNativeDpsWnd->RefreshRows();
		NativeAutoLootTrace("DPS parser window created");
	}

	if (show && gNativeDpsWnd) {
		gNativeDpsWnd->pXWnd()->Show(1, 1);
	}
}

static void NativeTradeskillsEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (!gNativeTradeskillsWnd) {
		NativeAutoLootTrace("creating tradeskills helper window");
		gNativeTradeskillsWnd = new NativeTradeskillsWnd();
		NativeAutoLootTrace("tradeskills helper window created");
	}

	if (show && gNativeTradeskillsWnd) {
		gNativeTradeskillsWnd->pXWnd()->Show(1, 1);
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
	if (!RuleList) {
		return;
	}

	// Hold the filter list at 36px rows (CListWnd row height lives at +0x21C).
	__try {
		if (((DWORD*)RuleList)[0x21C / 4] != 36) {
			((DWORD*)RuleList)[0x21C / 4] = 36;
			((CXWnd*)RuleList)->Show(0, 1);
			((CXWnd*)RuleList)->Show(1, 1);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("rules row height apply faulted");
	}

	// Self-calibrate the Location origin from the first placed control.
	if (!PoolCalibrated && CalibButton) {
		__try {
			CXRect actual = ((CXWnd*)CalibButton)->GetScreenRect();
			const int dx = CalibIntendedX - (int)actual.A;
			const int dy = CalibIntendedY - (int)actual.B;
			if (dx >= -64 && dx <= 64 && dy >= -64 && dy <= 64) {
				PoolCorrectionX += dx;
				PoolCorrectionY += dy;
				PoolCalibrated = true;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			PoolCalibrated = true;
		}
	}

	int window_left = 0;
	int window_top = 0;
	bool have_origin = false;
	__try {
		CXRect window_rect = ((CXWnd*)this)->GetScreenRect();
		window_left = (int)window_rect.A;
		window_top = (int)window_rect.B;
		have_origin = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}

	if (!have_origin) {
		return;
	}

	bool check_used[kAALRulePoolRows][4] = {};
	bool remove_used[kAALRulePoolRows] = {};
	bool icon_used[kAALRulePoolRows] = {};
	for (int pool_row = 0; pool_row < kAALRulePoolRows; ++pool_row) {
		RulePoolListRow[pool_row] = -1;
	}
	RulesRefresh.clear();

	// Pool rows are assigned to whichever list rows are currently visible
	// (scroll-aware, like the main window's SyncInlinePool); a row's
	// visibility is probed with its AN cell, and rows outside the clip
	// region are skipped without consuming a pool slot.
	const int columns_for_slot[4] = { 2, 3, 4, 5 };
	const int rule_rows = (int)gNativeAutoLootRuleRows.size();
	int pool_cursor = 0;
	for (int row = 0; row < RowCount && row < rule_rows; ++row) {
		if (pool_cursor >= kAALRulePoolRows) {
			break;
		}

		CXRect target;
		CXRect clip;
		if (!NativeAutoLootCellControlRect(RuleList, row, columns_for_slot[0], 24, false, &target, &clip)) {
			continue;
		}

		const int pool_row = pool_cursor++;
		RulePoolListRow[pool_row] = row;
		const NativeAutoLootRuleRow& rule = gNativeAutoLootRuleRows[row];
		const bool cell_active[4] = {
			rule.rule == "always_need",
			rule.rule == "always_greed",
			rule.rule == "never",
			rule.auto_roll
		};

		for (int slot = 0; slot < 4; ++slot) {
			CButtonWnd* button = CheckPool[pool_row][slot];
			if (!button) {
				continue;
			}

			if (!NativeAutoLootCellControlRect(RuleList, row, columns_for_slot[slot], 24, false, &target, &clip)) {
				continue;
			}

			PCSIDLWND raw = (PCSIDLWND)button;
			const int new_left = (int)target.A - window_left + PoolCorrectionX;
			const int new_top = (int)target.B - window_top + PoolCorrectionY;
			const bool moved = raw->Location.left != new_left || raw->Location.top != new_top;
			raw->Location.left = new_left;
			raw->Location.top = new_top;
			raw->Location.right = new_left + 24;
			raw->Location.bottom = new_top + 24;
			button->Checked = cell_active[slot] ? 1 : 0;
			if (moved) {
				RulesRefresh.push_back(button);
			}

			if (!CalibButton) {
				CalibButton = button;
				CalibIntendedX = (int)target.A;
				CalibIntendedY = (int)target.B;
			}

			check_used[pool_row][slot] = true;
		}

		CButtonWnd* remove_button = RemovePool[pool_row];
		if (remove_button && NativeAutoLootCellControlRect(RuleList, row, 6, 26, false, &target, &clip)) {
			PCSIDLWND raw = (PCSIDLWND)remove_button;
			const int new_left = (int)target.A - window_left + PoolCorrectionX;
			const int new_top = (int)target.B - window_top + PoolCorrectionY;
			const bool moved = raw->Location.left != new_left || raw->Location.top != new_top;
			raw->Location.left = new_left;
			raw->Location.top = new_top;
			raw->Location.right = new_left + 26;
			raw->Location.bottom = new_top + 26;
			if (moved) {
				RulesRefresh.push_back(remove_button);
			}

			remove_used[pool_row] = true;
		}

		CButtonWnd* icon_button = IconPool[pool_row];
		if (icon_button && row < (int)RowIconIds.size() &&
			NativeAutoLootCellControlRect(RuleList, row, 0, 26, true, &target, &clip)) {
			PCSIDLWND raw = (PCSIDLWND)icon_button;
			const int new_left = (int)target.A - window_left + PoolCorrectionX;
			const int new_top = (int)target.B - window_top + PoolCorrectionY;
			const bool moved = raw->Location.left != new_left || raw->Location.top != new_top;
			raw->Location.left = new_left;
			raw->Location.top = new_top;
			raw->Location.right = new_left + 26;
			raw->Location.bottom = new_top + 26;
			if (moved) {
				RulesRefresh.push_back(icon_button);
			}

			if (!gNativeAutoLootIconCellFaulted) {
				const int cell = NativeAutoLootIconCell(RowIconIds[row]);
				if (IconCell[pool_row] != cell) {
					DWORD* braw = (DWORD*)icon_button;
					CTextureAnimation* decal = (CTextureAnimation*)braw[kAALButtonNormalDecalOffset / 4];
					if (decal) {
						__try {
							decal->SetCurCell(cell);
							IconCell[pool_row] = cell;
							RulesRefresh.push_back(icon_button);
						}
						__except (EXCEPTION_EXECUTE_HANDLER) {
							gNativeAutoLootIconCellFaulted = true;
							NativeAutoLootTrace("rules icon SetCurCell faulted");
						}
					}
				}
			}

			icon_used[pool_row] = true;
		}
	}

	__try {
		for (int pool_row = 0; pool_row < kAALRulePoolRows; ++pool_row) {
			for (int slot = 0; slot < 4; ++slot) {
				CButtonWnd* button = CheckPool[pool_row][slot];
				if (button && check_used[pool_row][slot] != CheckShown[pool_row][slot]) {
					((CXWnd*)button)->Show(check_used[pool_row][slot] ? 1 : 0, 1);
					CheckShown[pool_row][slot] = check_used[pool_row][slot];
				}
			}

			if (RemovePool[pool_row] && remove_used[pool_row] != RemoveShown[pool_row]) {
				((CXWnd*)RemovePool[pool_row])->Show(remove_used[pool_row] ? 1 : 0, 1);
				RemoveShown[pool_row] = remove_used[pool_row];
			}

			if (IconPool[pool_row] && icon_used[pool_row] != IconShown[pool_row]) {
				((CXWnd*)IconPool[pool_row])->Show(icon_used[pool_row] ? 1 : 0, 1);
				IconShown[pool_row] = icon_used[pool_row];
			}
		}

		for (CButtonWnd* button : RulesRefresh) {
			((CXWnd*)button)->Show(0, 1);
			((CXWnd*)button)->Show(1, 1);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("rules pool show toggle faulted");
	}
}

void NativeAutoLootRulesWnd::HandleRuleCell(int pool_row, int slot)
{
	if (!RuleList || pool_row < 0 || pool_row >= kAALRulePoolRows) {
		return;
	}

	// Pool controls float over visible rows; resolve the clicked pool row
	// to the list row it is currently parked on.
	const int list_row = RulePoolListRow[pool_row];
	if (list_row < 0) {
		return;
	}

	const int item_id = (int)RuleList->GetItemData(list_row);
	if (item_id <= 0) {
		return;
	}

	NativeAutoLootRuleRow* rule = nullptr;
	for (NativeAutoLootRuleRow& row : gNativeAutoLootRuleRows) {
		if (row.item_id == item_id) {
			rule = &row;
			break;
		}
	}

	if (!rule) {
		return;
	}

	char command[128];
	if (slot == 4) {
		sprintf_s(command, "/say #advloot filter remove %d", item_id);
		NativeAutoLootSendCommand(command);
		SetStatus("Requested filter removal.");
		return;
	}

	if (slot == 3) {
		sprintf_s(command, "/say #advloot filter autoroll %d %s", item_id, rule->auto_roll ? "off" : "on");
		NativeAutoLootSendCommand(command);
		SetStatus("Toggled Auto Roll.");
		return;
	}

	static const char* kRuleNames[3] = { "always_need", "always_greed", "never" };
	const bool active =
		(slot == 0 && rule->rule == "always_need") ||
		(slot == 1 && rule->rule == "always_greed") ||
		(slot == 2 && rule->rule == "never");
	if (active) {
		sprintf_s(command, "/say #advloot filter remove %d", item_id);
		NativeAutoLootSendCommand(command);
		SetStatus("Requested rule removal.");
	}
	else {
		sprintf_s(command, "/say #advloot filter set %d %s", item_id, kRuleNames[slot]);
		NativeAutoLootSendCommand(command);
		SetStatus("Requested rule change.");
	}
}

void NativeAutoLootRulesWnd::RefreshRows()
{
	if (!RuleList) {
		return;
	}

	RuleList->DeleteAll();
	RowCount = 0;
	RowIconIds.clear();

	char summary[128];
	sprintf_s(
		summary,
		"Filters AN %d / AG %d / NV %d / Auto Roll %d",
		gNativeAutoLootAlwaysNeedCount,
		gNativeAutoLootAlwaysGreedCount,
		gNativeAutoLootNeverCount,
		gNativeAutoLootAutoRollCount
	);
	SetLabel(SummaryLabel, summary);

	SetButtonCheck(TglEnabled, gNativeAutoLootEnabled);
	SetButtonCheck(TglSplit, gNativeAutoLootAutoSplit);
	SetButtonCheck(TglConfirm, gNativeAutoLootConfirmRemove);
	SetButtonCheck(TglLore, gNativeAutoLootAutoRemoveLore);
	SetButtonCheck(TglShow, gNativeAutoLootAutoShow);
	SetButtonCheck(TglLootAll, gNativeAutoLootAutoLootAll);

	if (gNativeAutoLootRuleRows.empty()) {
		CXStr blank("");
		const int row = RuleList->AddString(blank, 0xFFB0B0B0, 0, nullptr, nullptr);
		CXStr empty("No filters loaded.");
		RuleList->SetItemText(row, 1, &empty);
		return;
	}

	CXStr spacer(" ");
	for (const NativeAutoLootRuleRow& entry : gNativeAutoLootRuleRows) {
		CXStr blank("");
		const int row = RuleList->AddString(blank, 0xFFFFFFFF, (uint32_t)entry.item_id, nullptr, entry.item.c_str());
		CXStr item(entry.item.c_str());
		RuleList->SetItemText(row, 1, &item);
		for (int column = 2; column <= 6; ++column) {
			RuleList->SetItemText(row, column, &spacer);
		}

		RowIconIds.push_back(entry.icon_id);
		++RowCount;
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

void NativeAutoLootSettingsWnd::SetLabel(CXWnd* label, const char* text)
{
	if (label) {
		CXStr value(text ? text : "");
		label->SetWindowTextA(value);
	}
}

void NativeAutoLootSettingsWnd::SetButtonCheck(CButtonWnd* button, bool checked)
{
	if (button) {
		button->Checked = checked ? 1 : 0;
		button->SetCheck(checked);
	}
}

void NativeAutoLootSettingsWnd::Layout()
{
	// Settings resize is handled by SIDL anchors.
}

void NativeAutoLootSettingsWnd::RefreshRows()
{
	char summary[160];
	sprintf_s(
		summary,
		"Advanced: %s  Filters: %s  Rules AN %d / AG %d / NV %d",
		gNativeAutoLootEnabled ? "on" : "off",
		gNativeAutoLootApplyFilters ? "on" : "off",
		gNativeAutoLootAlwaysNeedCount,
		gNativeAutoLootAlwaysGreedCount,
		gNativeAutoLootNeverCount
	);
	SetLabel(SummaryLabel, summary);

	char group_summary[192];
	sprintf_s(
		group_summary,
		"Master %s / Split %s / Loot All %s / Show %s / Lore %s",
		gNativeAutoLootMasterCandidate ? "on" : "off",
		gNativeAutoLootAutoSplit ? "on" : "off",
		gNativeAutoLootAutoLootAll ? "on" : "off",
		gNativeAutoLootAutoShow ? (gNativeAutoLootShowNewOnly ? "unfiltered" : "all") : "off",
		gNativeAutoLootAutoRemoveLore ? "on" : "off"
	);
	SetLabel(GroupSummaryLabel, group_summary);

	SetButtonCheck(AutoLootCheck, gNativeAutoLootEnabled);
	SetButtonCheck(NeedGreedCheck, gNativeAutoLootMasterCandidate);
	SetButtonCheck(AutoShowCheck, gNativeAutoLootAutoShow);
	SetButtonCheck(ShowNewCheck, gNativeAutoLootShowNewOnly);
	SetButtonCheck(ConfirmRemoveCheck, gNativeAutoLootConfirmRemove);
	SetButtonCheck(AutoRemoveLoreCheck, gNativeAutoLootAutoRemoveLore);
}

static void NativeAutoLootShowSettingsWindow()
{
	if (!gNativeAutoLootSettingsWnd) {
		NativeAutoLootTrace("creating settings window");
		gNativeAutoLootSettingsWnd = new NativeAutoLootSettingsWnd();
	}

	gNativeAutoLootSettingsWnd->RefreshRows();
	gNativeAutoLootSettingsWnd->pXWnd()->Show(1, 1);
	gNativeAutoLootSettingsWnd->SetStatus("Refreshing Advanced Loot settings...");
	NativeAutoLootSendCommand("/say #advloot native status");
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
	NativeAutoLootSendCommand("/say #advloot filter native list");
}

static void NativeAutoLootShowManageWindow(int entry_id)
{
	if (!gNativeAutoLootManageWnd) {
		NativeAutoLootTrace("creating manage window");
		gNativeAutoLootManageWnd = new NativeAutoLootManageWnd();
	}

	gNativeAutoLootManageEntryId = entry_id;
	gNativeAutoLootManagePlayers.clear();
	gNativeAutoLootManageWnd->RefreshRows();
	gNativeAutoLootManageWnd->pXWnd()->Show(1, 1);
	gNativeAutoLootManageWnd->RequestRefresh();
}

static void NativeAutoLootEnsureWindow(bool show)
{
	if (!pSidlMgr || !pWndMgr) {
		return;
	}

	if (gNativeAutoLootWindowConstructionFaulted) {
		return;
	}

	if (!gNativeAutoLootWnd) {
		NativeAutoLootTrace("creating window");
		NativeAutoLootWnd* created_window = nullptr;
		__try {
			created_window = new NativeAutoLootWnd();
			if (created_window) {
				created_window->RefreshRows();
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("AutoLoot window construction faulted; check EQUI_NativeAutoLootWnd.xml");
			created_window = nullptr;
		}

		if (!created_window) {
			gNativeAutoLootWindowConstructionFaulted = true;
			return;
		}

		gNativeAutoLootWnd = created_window;
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

bool NativeAutoLootWnd::SendListAction(const NativeAutoLootRow& row, const char* action, const char* status)
{
	if (row.entry_id <= 0 || !action || !action[0]) {
		return false;
	}

	char command[128];
	sprintf_s(command, "/say #advloot action %d %s", row.entry_id, action);
	NativeAutoLootSendCommand(command);
	SetStatus(status && status[0] ? status : "Sent Advanced Loot action.");
	return true;
}

bool NativeAutoLootWnd::SendCorpseAction(const NativeAutoLootRow& row, const char* action, const char* status)
{
	if (row.entry_id <= 0 || !action || !action[0]) {
		return false;
	}

	char command[128];
	sprintf_s(command, "/say #advloot corpse %s %d", action, row.entry_id);
	NativeAutoLootSendCommand(command);
	SetStatus(status && status[0] ? status : "Sent corpse action.");
	return true;
}

bool NativeAutoLootWnd::ToggleAutoRollFilter(const NativeAutoLootRow& row)
{
	if (row.item_id <= 0) {
		SetStatus("That row does not have a filterable item.");
		return true;
	}

	char command[128];
	sprintf_s(command, "/say #advloot filter autoroll %d %s", row.item_id, row.auto_roll ? "off" : "on");
	NativeAutoLootSendCommand(command);
	SetStatus(row.auto_roll ? "Turned Auto Roll off for this item." : "Turned Auto Roll on for this item.");
	return true;
}

int NativeAutoLootWnd::ResolveComboChoice(CComboWnd* combo, int hint)
{
	// GetCurChoice is authoritative; the notification payload is only a
	// fallback because it reads 0 regardless of selection in this client.
	int resolved = -1;
	__try {
		resolved = combo ? combo->GetCurChoice() : -1;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("combo GetCurChoice faulted");
	}

	if (resolved >= 0 && resolved <= 8) {
		return resolved;
	}

	return (hint >= 0 && hint <= 8) ? hint : -1;
}

void NativeAutoLootWnd::ApplySetAll(bool shared, int choice)
{
	NativeAutoLootTrace("DIAG set-all shared=%d choice=%d", shared ? 1 : 0, choice);

	if (!shared) {
		if (choice == 0) {
			NativeAutoLootSendCommand("/say #advloot personal lootall");
			SetStatus("Requested Loot All.");
			return;
		}

		if (choice == 1) {
			NativeAutoLootSendCommand("/say #advloot personal leaveall");
			SetStatus("Requested Leave All.");
			return;
		}
	}

	const char* action = nullptr;
	if (!shared) {
		action = choice == 2 ? "alwaysneed" : choice == 3 ? "alwaysgreed" : choice == 4 ? "never" : nullptr;
	}
	else {
		action = choice == 0 ? "need" : choice == 1 ? "greed" : choice == 2 ? "no" :
			choice == 3 ? "alwaysneed" : choice == 4 ? "alwaysgreed" : choice == 5 ? "never" :
			choice == 6 ? "leave" : nullptr;
	}

	if (!action) {
		return;
	}

	int sent = 0;
	for (const NativeAutoLootRow& row : gNativeAutoLootRows) {
		if (row.shared != shared || row.entry_id <= 0) {
			continue;
		}

		char command[128];
		sprintf_s(command, "/say #advloot action %d %s", row.entry_id, action);
		NativeAutoLootSendCommand(command);
		++sent;
	}

	char status[96];
	sprintf_s(status, "Applied %s to %d item%s.", action, sent, sent == 1 ? "" : "s");
	SetStatus(status);
}

bool NativeAutoLootWnd::GetInlineCellDrawRect(const NativeAutoLootInlineCellSpec& spec, CXRect* target, CXRect* clip) const
{
	if (!target || !clip || !spec.list) {
		return false;
	}

	__try {
		CXRect cell_rect = spec.list->GetItemRect(spec.row, spec.column);
		CXRect list_rect = ((CXWnd*)spec.list)->GetScreenRect();
		CXRect list_clip = ((CXWnd*)spec.list)->GetScreenClipRect();

		int left = (int)cell_rect.A;
		int top = (int)cell_rect.B;
		int right = (int)cell_rect.C;
		int bottom = (int)cell_rect.D;

		if (NativeAutoLootListRectIsRelative(spec.list, spec.row, spec.column, cell_rect, list_rect)) {
			left += (int)list_rect.A;
			right += (int)list_rect.A;
			top += (int)list_rect.B;
			bottom += (int)list_rect.B;
		}

		if (right <= left || bottom <= top ||
			right <= (int)list_clip.A || left >= (int)list_clip.C ||
			bottom <= (int)list_clip.B || top >= (int)list_clip.D) {
			return false;
		}

		const int control_size = NativeAutoLootPoolControlSize(spec.list == SharedList, spec.column);
		int horizontal_inset = (right - left - control_size) / 2;
		int vertical_inset = (bottom - top - control_size) / 2;
		if (horizontal_inset < 0) {
			horizontal_inset = 0;
		}
		if (vertical_inset < 0) {
			vertical_inset = 0;
		}

		int target_left = left + horizontal_inset;
		int target_top = top + vertical_inset;
		target->A = target_left;
		target->B = target_top;
		target->C = target_left + control_size;
		target->D = target_top + control_size;
		clip->A = list_clip.A;
		clip->B = list_clip.B;
		clip->C = list_clip.C;
		clip->D = list_clip.D;
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("Advanced Loot inline checkbox rect faulted");
		return false;
	}
}

bool NativeAutoLootWnd::GetIconCellDrawRect(CListWnd* list, int row, CXRect* target, CXRect* clip) const
{
	if (!target || !clip || !list) {
		return false;
	}

	__try {
		CXRect cell_rect = list->GetItemRect(row, 0);
		CXRect list_rect = ((CXWnd*)list)->GetScreenRect();
		CXRect list_clip = ((CXWnd*)list)->GetScreenClipRect();

		int left = (int)cell_rect.A;
		int top = (int)cell_rect.B;
		int right = (int)cell_rect.C;
		int bottom = (int)cell_rect.D;

		if (NativeAutoLootListRectIsRelative(list, row, 0, cell_rect, list_rect)) {
			left += (int)list_rect.A;
			right += (int)list_rect.A;
			top += (int)list_rect.B;
			bottom += (int)list_rect.B;
		}

		if (right <= left || bottom <= top ||
			right <= (int)list_clip.A || left >= (int)list_clip.C ||
			bottom <= (int)list_clip.B || top >= (int)list_clip.D) {
			return false;
		}

		const int control_size = 26;
		int vertical_inset = (bottom - top - control_size) / 2;
		if (vertical_inset < 0) {
			vertical_inset = 0;
		}

		target->A = left + 2;
		target->B = top + vertical_inset;
		target->C = left + 2 + control_size;
		target->D = top + vertical_inset + control_size;
		clip->A = list_clip.A;
		clip->B = list_clip.B;
		clip->C = list_clip.C;
		clip->D = list_clip.D;
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("icon cell rect faulted");
		return false;
	}
}

void NativeAutoLootWnd::SetIconButtonCell(CButtonWnd* button, int* last_cell, int icon_id)
{
	if (!button || !last_cell || gNativeAutoLootIconCellFaulted) {
		return;
	}

	const int cell = NativeAutoLootIconCell(icon_id);
	if (*last_cell == cell) {
		return;
	}

	DWORD* raw = (DWORD*)button;
	CTextureAnimation* decal = (CTextureAnimation*)raw[kAALButtonNormalDecalOffset / 4];
	if (!decal) {
		return;
	}

	__try {
		decal->SetCurCell(cell);
		*last_cell = cell;
		PoolRefresh.push_back(button);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		gNativeAutoLootIconCellFaulted = true;
		NativeAutoLootTrace("icon SetCurCell faulted; icon cells disabled");
	}
}

// CListWnd stores its fixed row height at +0x21C (located empirically via
// the 2026-06-09 stride-scan trials; stock value is 14). Hold it at 36 so
// rows fit the font-5 text, 24px checkboxes, and 26px item icons.
void NativeAutoLootWnd::ApplyListRowHeights()
{
	__try {
		bool changed = false;
		if (PersonalList && ((DWORD*)PersonalList)[0x21C / 4] != 36) {
			((DWORD*)PersonalList)[0x21C / 4] = 36;
			((CXWnd*)PersonalList)->Show(0, 1);
			((CXWnd*)PersonalList)->Show(1, 1);
			changed = true;
		}

		if (SharedList && ((DWORD*)SharedList)[0x21C / 4] != 36) {
			((DWORD*)SharedList)[0x21C / 4] = 36;
			((CXWnd*)SharedList)->Show(0, 1);
			((CXWnd*)SharedList)->Show(1, 1);
			changed = true;
		}

		if (changed) {
			NativeAutoLootTrace("applied 36px list row height");
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("row height apply faulted");
	}
}
void NativeAutoLootWnd::SyncInlinePool()
{
	// The engine interprets child Location rects relative to an origin that
	// may differ from the window's outer screen rect (e.g. below the title
	// bar). Measure where the first placed button actually rendered and
	// latch the delta as a uniform correction for all pool buttons.
	if (!PoolCalibrated && !PoolStates.empty()) {
		const NativeAutoLootPoolCellState& probe = PoolStates.front();
		__try {
			CXRect actual = ((CXWnd*)probe.button)->GetScreenRect();
			const int dx = probe.intended_left - (int)actual.A;
			const int dy = probe.intended_top - (int)actual.B;
			if (dx >= -64 && dx <= 64 && dy >= -64 && dy <= 64) {
				PoolCorrectionX += dx;
				PoolCorrectionY += dy;
				PoolCalibrated = true;
				if (dx || dy) {
					NativeAutoLootTrace("inline pool calibrated dx=%d dy=%d", dx, dy);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("inline pool calibration faulted");
			PoolCalibrated = true;
		}
	}

	PoolStates.clear();

	bool personal_used[kAALPoolPersonalRows][kAALPoolPersonalCols] = {};
	bool shared_used[kAALPoolSharedRows][kAALPoolSharedCols] = {};
	bool personal_icon_used[kAALPoolPersonalRows] = {};
	bool shared_icon_used[kAALPoolSharedRows] = {};
	for (int pool_row = 0; pool_row < kAALPoolPersonalRows; ++pool_row) {
		PersonalPoolListRow[pool_row] = -1;
	}
	for (int pool_row = 0; pool_row < kAALPoolSharedRows; ++pool_row) {
		SharedPoolListRow[pool_row] = -1;
	}
	PoolRefresh.clear();

	int window_left = 0;
	int window_top = 0;
	bool have_window_rect = false;
	__try {
		CXRect window_rect = ((CXWnd*)this)->GetScreenRect();
		window_left = (int)window_rect.A;
		window_top = (int)window_rect.B;
		have_window_rect = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("inline pool window rect faulted");
	}

	if (have_window_rect) {
		CListWnd* current_list = nullptr;
		int current_src_row = -1;
		int current_pool_row = -1;
		int pool_row_cursor = 0;

		for (const NativeAutoLootInlineCellSpec& spec : gNativeAutoLootInlineCellSpecs) {
			const bool shared = spec.list == SharedList;
			if (!shared && spec.list != PersonalList) {
				continue;
			}

			if (spec.list != current_list) {
				current_list = spec.list;
				current_src_row = -1;
				current_pool_row = -1;
				pool_row_cursor = 0;
			}

			CXRect target;
			CXRect clip;
			if (!GetInlineCellDrawRect(spec, &target, &clip)) {
				continue;
			}

			if ((int)target.B < (int)clip.B || (int)target.D > (int)clip.D) {
				continue;
			}

			const int pool_rows = shared ? kAALPoolSharedRows : kAALPoolPersonalRows;
			if (spec.row != current_src_row) {
				current_src_row = spec.row;
				current_pool_row = pool_row_cursor < pool_rows ? pool_row_cursor : -1;
				++pool_row_cursor;

				if (current_pool_row >= 0) {
					if (shared) {
						SharedPoolListRow[current_pool_row] = spec.row;
					}
					else {
						PersonalPoolListRow[current_pool_row] = spec.row;
					}
					CButtonWnd* icon_button = shared ? SharedIconPool[current_pool_row] : PersonalIconPool[current_pool_row];
					CXRect icon_target;
					CXRect icon_clip;
					if (icon_button &&
						GetIconCellDrawRect(spec.list, spec.row, &icon_target, &icon_clip) &&
						(int)icon_target.B >= (int)icon_clip.B && (int)icon_target.D <= (int)icon_clip.D) {
						const int icon_left = (int)icon_target.A - window_left + PoolCorrectionX;
						const int icon_top = (int)icon_target.B - window_top + PoolCorrectionY;
						const int icon_right = (int)icon_target.C - window_left + PoolCorrectionX;
						const int icon_bottom = (int)icon_target.D - window_top + PoolCorrectionY;

						PCSIDLWND icon_raw = (PCSIDLWND)icon_button;
						const bool icon_moved =
							icon_raw->Location.left != icon_left ||
							icon_raw->Location.top != icon_top ||
							icon_raw->Location.right != icon_right ||
							icon_raw->Location.bottom != icon_bottom;
						icon_raw->Location.left = icon_left;
						icon_raw->Location.top = icon_top;
						icon_raw->Location.right = icon_right;
						icon_raw->Location.bottom = icon_bottom;
						if (icon_moved) {
							PoolRefresh.push_back(icon_button);
						}

						int* last_cell = shared ? &SharedIconCell[current_pool_row] : &PersonalIconCell[current_pool_row];
						SetIconButtonCell(icon_button, last_cell, NativeAutoLootIconIdForEntry((int)spec.list->GetItemData(spec.row)));

						if (shared) {
							shared_icon_used[current_pool_row] = true;
						}
						else {
							personal_icon_used[current_pool_row] = true;
						}
					}
				}
			}

			if (current_pool_row < 0) {
				continue;
			}

			const int slot = NativeAutoLootPoolSlotForColumn(shared, spec.column);
			if (slot < 0) {
				continue;
			}

			CButtonWnd* button = shared ? SharedPool[current_pool_row][slot] : PersonalPool[current_pool_row][slot];
			if (!button) {
				continue;
			}

			const int new_left = (int)target.A - window_left + PoolCorrectionX;
			const int new_top = (int)target.B - window_top + PoolCorrectionY;
			const int new_right = (int)target.C - window_left + PoolCorrectionX;
			const int new_bottom = (int)target.D - window_top + PoolCorrectionY;

			PCSIDLWND raw = (PCSIDLWND)button;
			const bool moved =
				raw->Location.left != new_left ||
				raw->Location.top != new_top ||
				raw->Location.right != new_right ||
				raw->Location.bottom != new_bottom;
			raw->Location.left = new_left;
			raw->Location.top = new_top;
			raw->Location.right = new_right;
			raw->Location.bottom = new_bottom;
			button->Checked = spec.active ? 1 : 0;

			if (moved) {
				PoolRefresh.push_back(button);
			}

			if (shared) {
				shared_used[current_pool_row][slot] = true;
			}
			else {
				personal_used[current_pool_row][slot] = true;
			}

			NativeAutoLootPoolCellState state = { button, spec.list, spec.row, spec.column, spec.enabled, (int)target.A, (int)target.B };
			PoolStates.push_back(state);
		}
	}

	__try {
		for (int pool_row = 0; pool_row < kAALPoolPersonalRows; ++pool_row) {
			for (int slot = 0; slot < kAALPoolPersonalCols; ++slot) {
				CButtonWnd* button = PersonalPool[pool_row][slot];
				if (button && personal_used[pool_row][slot] != PersonalPoolShown[pool_row][slot]) {
					((CXWnd*)button)->Show(personal_used[pool_row][slot] ? 1 : 0, 1);
					PersonalPoolShown[pool_row][slot] = personal_used[pool_row][slot];
				}
			}
		}

		for (int pool_row = 0; pool_row < kAALPoolSharedRows; ++pool_row) {
			for (int slot = 0; slot < kAALPoolSharedCols; ++slot) {
				CButtonWnd* button = SharedPool[pool_row][slot];
				if (button && shared_used[pool_row][slot] != SharedPoolShown[pool_row][slot]) {
					((CXWnd*)button)->Show(shared_used[pool_row][slot] ? 1 : 0, 1);
					SharedPoolShown[pool_row][slot] = shared_used[pool_row][slot];
				}
			}
		}

		for (int pool_row = 0; pool_row < kAALPoolPersonalRows; ++pool_row) {
			CButtonWnd* button = PersonalIconPool[pool_row];
			if (button && personal_icon_used[pool_row] != PersonalIconShown[pool_row]) {
				((CXWnd*)button)->Show(personal_icon_used[pool_row] ? 1 : 0, 1);
				PersonalIconShown[pool_row] = personal_icon_used[pool_row];
			}
		}

		for (int pool_row = 0; pool_row < kAALPoolSharedRows; ++pool_row) {
			CButtonWnd* button = SharedIconPool[pool_row];
			if (button && shared_icon_used[pool_row] != SharedIconShown[pool_row]) {
				((CXWnd*)button)->Show(shared_icon_used[pool_row] ? 1 : 0, 1);
				SharedIconShown[pool_row] = shared_icon_used[pool_row];
			}
		}

		// The engine caches a control's absolute screen position and only
		// recomputes it on events like a hide/show transition or a window
		// move. Toggle visibility within the pulse (between frames) so
		// Location writes take effect immediately.
		for (CButtonWnd* button : PoolRefresh) {
			((CXWnd*)button)->Show(0, 1);
			((CXWnd*)button)->Show(1, 1);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		NativeAutoLootTrace("inline pool show toggle faulted");
	}
}

void NativeAutoLootWnd::DiagnosticPulse()
{
	++gNativeAutoLootDiagPulseCount;

	SyncInlinePool();

	ApplyListRowHeights();

	const DWORD now = GetTickCount();
	if (gNativeAutoLootDiagReportBudget <= 0 || (now - gNativeAutoLootDiagLastReportTick) < 2000) {
		return;
	}

	gNativeAutoLootDiagLastReportTick = now;
	--gNativeAutoLootDiagReportBudget;

	char pool_text[96];
	sprintf_s(pool_text, "pool:%u", (unsigned int)PoolStates.size());
	if (!PoolStates.empty()) {
		PCSIDLWND raw = (PCSIDLWND)PoolStates.front().button;
		sprintf_s(pool_text, "pool:%u first:(%d,%d,%d,%d)",
			(unsigned int)PoolStates.size(),
			(int)raw->Location.left, (int)raw->Location.top,
			(int)raw->Location.right, (int)raw->Location.bottom);
	}

	char report[256];
	sprintf_s(report, "DIAG P:%u PD:%u OPF:%u specs:%u %s corr:(%d,%d,%s)",
		gNativeAutoLootDiagPulseCount,
		gNativeAutoLootDiagPostDrawCount,
		gNativeAutoLootDiagOnProcessFrameCount,
		(unsigned int)gNativeAutoLootInlineCellSpecs.size(),
		pool_text,
		PoolCorrectionX,
		PoolCorrectionY,
		PoolCalibrated ? "ok" : "pending");

	NativeAutoLootTrace("%s", report);
}

bool NativeAutoLootWnd::HandleListColumnClick(CListWnd* list, bool shared, void* hit_test_point)
{
	if (!list) {
		return false;
	}

	int selected = list->GetCurSel();
	int column = -1;
	if (NativeAutoLootTryGetClickedCell(list, hit_test_point, &selected, &column)) {
		list->SetCurSel(selected);
	}

	if (selected < 0 || column < 0) {
		return false;
	}

	return HandleListColumnAction(list, shared, selected, column);
}

bool NativeAutoLootWnd::HandleListColumnAction(CListWnd* list, bool shared, int selected, int column)
{
	if (!list || selected < 0 || column < 0) {
		return false;
	}

	const int entry_id = (int)list->GetItemData(selected);
	if (entry_id <= 0) {
		return false;
	}

	const NativeAutoLootRow* row = nullptr;
	for (const NativeAutoLootRow& candidate : gNativeAutoLootRows) {
		if (candidate.entry_id == entry_id) {
			row = &candidate;
			break;
		}
	}

	if (!row) {
		return false;
	}

	if (!shared) {
		switch (column) {
		case kAALPersonalLoot:
			return SendListAction(*row, "loot", "Requested loot.");
		case kAALPersonalLeave:
			return SendListAction(*row, "leave", "Requested leave.");
		case kAALPersonalAlwaysNeed:
			return SendListAction(*row, "alwaysneed", "Marked Always Need.");
		case kAALPersonalAlwaysGreed:
			return SendListAction(*row, "alwaysgreed", "Marked Always Greed.");
		case kAALPersonalNever:
			return SendListAction(*row, "never", "Marked Never and left item.");
		default:
			break;
		}
	}
	else {
		switch (column) {
		case kAALSharedStatus:
			if (row->locked) {
				SetStatus(row->lock_reason.empty() ? "That corpse is locked." : row->lock_reason.c_str());
				return true;
			}
			if (row->free_grab && row->can_loot) {
				return SendListAction(*row, "loot", "Requested Free Grab loot.");
			}
			if (row->can_freegrab) {
				return SendListAction(*row, "freegrab", "Set Free Grab.");
			}
			SetStatus(row->free_grab ? "Free Grab is active." : "No status action is available.");
			return true;
		case kAALSharedAction:
			if (row->can_roll) {
				return SendListAction(*row, "roll", "Resolved roll.");
			}
			if (row->can_ask) {
				return SendListAction(*row, "ask", "Started Ask/Roll.");
			}
			SetStatus("No Ask/Roll action is available.");
			return true;
		case kAALSharedManage:
			if (row->manage || row->can_give) {
				NativeAutoLootShowManageWindow(row->entry_id);
				SetStatus("Opened Manage Loot.");
				return true;
			}
			SetStatus("Only the Master Looter can manage that row.");
			return true;
		case kAALSharedAutoRoll:
			return ToggleAutoRollFilter(*row);
		case kAALSharedNeed:
			if (!row->can_vote && !NativeAutoLootIsNeedVote(*row)) {
				SetStatus("Need is available once Ask/Roll starts.");
				return true;
			}
			return SendListAction(*row, "need", "Marked Need.");
		case kAALSharedGreed:
			if (!row->can_vote && !NativeAutoLootIsGreedVote(*row)) {
				SetStatus("Greed is available once Ask/Roll starts.");
				return true;
			}
			return SendListAction(*row, "greed", "Marked Greed.");
		case kAALSharedNo:
			if (!row->can_vote && !NativeAutoLootIsNoVote(*row)) {
				SetStatus("No is available once Ask/Roll starts.");
				return true;
			}
			return SendListAction(*row, "no", "Marked No.");
		case kAALSharedAlwaysNeed:
			return SendListAction(*row, "alwaysneed", "Marked Always Need.");
		case kAALSharedAlwaysGreed:
			return SendListAction(*row, "alwaysgreed", "Marked Always Greed.");
		case kAALSharedNever:
			return SendListAction(*row, "never", "Marked Never.");
		case kAALSharedSource:
			if (GetAsyncKeyState(VK_MENU) & 0x8000) {
				return SendCorpseAction(*row, "link", "Linked corpse loot.");
			}
			return SendCorpseAction(*row, "target", "Selected corpse.");
		default:
			break;
		}
	}

	return false;
}

void NativeAutoLootWnd::Layout()
{
	// Only refit columns when the list width actually changes: re-applying
	// widths every pulse can oscillate cell rects when the scrollbar
	// appears/disappears, which thrashes the overlay buttons.
	const int personal_width = NativeAutoLootGetListWidth(PersonalList);
	if (personal_width > 0 && personal_width != LastLayoutWidth) {
		LastLayoutWidth = personal_width;
		NativeAutoLootFitListColumns(PersonalList, false);
	}

	const int shared_width = NativeAutoLootGetListWidth(SharedList);
	if (shared_width > 0 && shared_width != LastLayoutHeight) {
		LastLayoutHeight = shared_width;
		NativeAutoLootFitListColumns(SharedList, true);
	}
}

void NativeAutoLootWnd::RefreshList(CListWnd* list, bool shared)
{
	if (!list) {
		return;
	}

	list->DeleteAll();
	int visible = 0;

	std::vector<const NativeAutoLootRow*> rows;
	for (const NativeAutoLootRow& entry : gNativeAutoLootRows) {
		if (entry.shared != shared) {
			continue;
		}

		rows.push_back(&entry);
	}

	if (gNativeAutoLootGroupByNpcDisplay) {
		std::sort(rows.begin(), rows.end(), [](const NativeAutoLootRow* left, const NativeAutoLootRow* right) {
			if (left->source != right->source) {
				return left->source < right->source;
			}

			if (left->item != right->item) {
				return left->item < right->item;
			}

			return left->entry_id < right->entry_id;
		});
	}

	for (const NativeAutoLootRow* entry_ptr : rows) {
		const NativeAutoLootRow& entry = *entry_ptr;
		++visible;
		const COLORREF row_color = entry.locked ? 0xFFFF8080 : 0xFFFFFFFF;
		char display_name[160];
		sprintf_s(display_name, "      %s", entry.item.c_str());
		CXStr item(display_name);
		const int row = list->AddString(
			item,
			row_color,
			(uint32_t)entry.entry_id,
			nullptr,
			entry.item.c_str()
		);

		if (!shared) {
			const bool always_need = entry.rule == "always_need" || entry.state == "alwaysneed";
			const bool always_greed = entry.rule == "always_greed" || entry.state == "alwaysgreed";
			CXStr source(entry.source.c_str());
			NativeAutoLootSetSquareCell(list, row, kAALPersonalLoot, false, entry.can_loot && !entry.locked, false);
			NativeAutoLootSetSquareCell(list, row, kAALPersonalLeave, false, entry.can_leave && !entry.locked, true);
			NativeAutoLootSetSquareCell(list, row, kAALPersonalAlwaysNeed, always_need, true, false);
			NativeAutoLootSetSquareCell(list, row, kAALPersonalAlwaysGreed, always_greed, true, false);
			NativeAutoLootSetSquareCell(list, row, kAALPersonalNever, entry.rule == "never", true, true);
			list->SetItemText(row, kAALPersonalSource, &source);
			list->SetItemColor(row, kAALPersonalLoot, NativeAutoLootSquareColor(false, entry.can_loot && !entry.locked, 0xFF66FF66));
			list->SetItemColor(row, kAALPersonalLeave, NativeAutoLootSquareColor(false, entry.can_leave && !entry.locked, 0xFFFF8080));
			list->SetItemColor(row, kAALPersonalAlwaysNeed, NativeAutoLootSquareColor(always_need, true, 0xFF66FF66));
			list->SetItemColor(row, kAALPersonalAlwaysGreed, NativeAutoLootSquareColor(always_greed, true, 0xFFFFFF80));
			list->SetItemColor(row, kAALPersonalNever, NativeAutoLootSquareColor(entry.rule == "never", true, 0xFFFF8080));
			list->SetItemColor(row, kAALPersonalSource, entry.locked ? 0xFFFF8080 : 0xFFFFFFFF);
		}
		else {
			char status_text[96];
			const std::string status_kind = entry.status_kind.empty() ? entry.state : entry.status_kind;
			if (entry.locked) {
				sprintf_s(status_text, "Locked");
			}
			else if (entry.free_grab) {
				sprintf_s(status_text, "Free Grab");
			}
			else if (entry.roll_seconds > 0) {
				const char* phase = status_kind == "rolling" ? "Roll" : "Ask";
				sprintf_s(status_text, "%s %ds", phase, entry.roll_seconds);
			}
			else if (entry.need_count || entry.greed_count || entry.no_count || entry.waiting_count) {
				sprintf_s(status_text, "N%d G%d No%d W%d", entry.need_count, entry.greed_count, entry.no_count, entry.waiting_count);
			}
			else {
				sprintf_s(status_text, "%s", entry.state.c_str());
			}
			CXStr source(entry.source.c_str());
			CXStr status(status_text);
			const bool need = NativeAutoLootIsNeedVote(entry);
			const bool greed = NativeAutoLootIsGreedVote(entry);
			const bool no = NativeAutoLootIsNoVote(entry);
			const bool always_need = entry.rule == "always_need";
			const bool always_greed = entry.rule == "always_greed";
			const bool never = entry.rule == "never";
			const bool filter_enabled = entry.item_id > 0;
			const bool vote_enabled = entry.can_vote && !entry.locked;
			const bool action_available = (entry.can_roll || entry.can_ask) && !entry.locked;
			const bool manage_available = (entry.manage || entry.can_give) && !entry.locked;
			const bool grab_available = !entry.locked && ((entry.free_grab && entry.can_loot) || entry.can_freegrab);

			if (grab_available) {
				NativeAutoLootSetSquareCell(list, row, kAALSharedStatus, entry.free_grab, true, false);
			}
			else {
				list->SetItemText(row, kAALSharedStatus, &status);
			}

			if (action_available) {
				NativeAutoLootSetSquareCell(list, row, kAALSharedAction, false, true, false);
			}
			else {
				CXStr action("-");
				list->SetItemText(row, kAALSharedAction, &action);
			}

			if (manage_available) {
				NativeAutoLootSetSquareCell(list, row, kAALSharedManage, false, true, false);
			}
			else {
				CXStr manage("-");
				list->SetItemText(row, kAALSharedManage, &manage);
			}

			NativeAutoLootSetSquareCell(list, row, kAALSharedAutoRoll, entry.auto_roll, filter_enabled, false);
			NativeAutoLootSetSquareCell(list, row, kAALSharedNeed, need, vote_enabled || need, false);
			NativeAutoLootSetSquareCell(list, row, kAALSharedGreed, greed, vote_enabled || greed, false);
			NativeAutoLootSetSquareCell(list, row, kAALSharedNo, no, vote_enabled || no, true);
			NativeAutoLootSetSquareCell(list, row, kAALSharedAlwaysNeed, always_need, filter_enabled, false);
			NativeAutoLootSetSquareCell(list, row, kAALSharedAlwaysGreed, always_greed, filter_enabled, false);
			NativeAutoLootSetSquareCell(list, row, kAALSharedNever, never, filter_enabled, true);
			list->SetItemText(row, kAALSharedSource, &source);
			list->SetItemColor(row, kAALSharedStatus, entry.locked ? 0xFFFF8080 : entry.free_grab ? 0xFF80D0FF : 0xFFFFFFFF);
			list->SetItemColor(row, kAALSharedAction, (entry.can_ask || entry.can_roll) && !entry.locked ? 0xFFB8D8FF : 0xFF606060);
			list->SetItemColor(row, kAALSharedManage, (entry.manage || entry.can_give) && !entry.locked ? 0xFFB8D8FF : 0xFF606060);
			list->SetItemColor(row, kAALSharedAutoRoll, NativeAutoLootSquareColor(entry.auto_roll, filter_enabled, 0xFF80D0FF));
			list->SetItemColor(row, kAALSharedNeed, NativeAutoLootSquareColor(need, vote_enabled, 0xFF66FF66));
			list->SetItemColor(row, kAALSharedGreed, NativeAutoLootSquareColor(greed, vote_enabled, 0xFFFFFF80));
			list->SetItemColor(row, kAALSharedNo, NativeAutoLootSquareColor(no, vote_enabled, 0xFFFF8080));
			list->SetItemColor(row, kAALSharedAlwaysNeed, NativeAutoLootSquareColor(always_need, filter_enabled, 0xFF66FF66));
			list->SetItemColor(row, kAALSharedAlwaysGreed, NativeAutoLootSquareColor(always_greed, filter_enabled, 0xFFFFFF80));
			list->SetItemColor(row, kAALSharedNever, NativeAutoLootSquareColor(never, filter_enabled, 0xFFFF8080));
			list->SetItemColor(row, kAALSharedSource, row_color);
		}
	}

	if (visible) {
		return;
	}

	CXStr dash("-");
	const int row = list->AddString(dash, 0xFFB0B0B0, 0, nullptr, nullptr);
	if (!shared) {
		CXStr empty("No personal loot.");
		list->SetItemText(row, kAALPersonalLoot, &dash);
		list->SetItemText(row, kAALPersonalLeave, &dash);
		list->SetItemText(row, kAALPersonalAlwaysNeed, &dash);
		list->SetItemText(row, kAALPersonalAlwaysGreed, &dash);
		list->SetItemText(row, kAALPersonalNever, &dash);
		list->SetItemText(row, kAALPersonalSource, &empty);
	}
	else {
		CXStr empty(gNativeAutoLootGrouped ? "No shared loot." : "Not grouped.");
		list->SetItemText(row, kAALSharedStatus, &dash);
		list->SetItemText(row, kAALSharedAction, &dash);
		list->SetItemText(row, kAALSharedManage, &dash);
		list->SetItemText(row, kAALSharedAutoRoll, &dash);
		list->SetItemText(row, kAALSharedNeed, &dash);
		list->SetItemText(row, kAALSharedGreed, &dash);
		list->SetItemText(row, kAALSharedNo, &dash);
		list->SetItemText(row, kAALSharedAlwaysNeed, &dash);
		list->SetItemText(row, kAALSharedAlwaysGreed, &dash);
		list->SetItemText(row, kAALSharedNever, &dash);
		list->SetItemText(row, kAALSharedSource, &empty);
	}
}

void NativeAutoLootWnd::RefreshRows()
{
	gNativeAutoLootInlineCellSpecs.clear();
	RefreshList(PersonalList, false);
	RefreshList(SharedList, true);

	char rules[128];
	sprintf_s(
		rules,
		"Rules AN %d / AG %d / NV %d",
		gNativeAutoLootAlwaysNeedCount,
		gNativeAutoLootAlwaysGreedCount,
		gNativeAutoLootNeverCount
	);
	SetLabel(RuleSummaryLabel, rules);

	char master[192];
	sprintf_s(
		master,
		"Shared Loot: %s  Master: %s  Candidate: %s  Auto Roll Filters: %d",
		gNativeAutoLootGrouped ? "grouped" : "solo",
		gNativeAutoLootMasterName.empty() ? "-" : gNativeAutoLootMasterName.c_str(),
		gNativeAutoLootMasterCandidate ? "on" : "off",
		gNativeAutoLootAutoRollCount
	);
	SetLabel(MasterLabel, master);

	if (ApplyFiltersCheck) {
		ApplyFiltersCheck->Checked = gNativeAutoLootApplyFilters ? 1 : 0;
		ApplyFiltersCheck->SetCheck(gNativeAutoLootApplyFilters);
		CXStr value("Apply Filters");
		((CXWnd*)ApplyFiltersCheck)->SetWindowTextA(value);
	}

	if (GroupedByNpcCheck) {
		GroupedByNpcCheck->Checked = gNativeAutoLootGroupByNpcDisplay ? 1 : 0;
		GroupedByNpcCheck->SetCheck(gNativeAutoLootGroupByNpcDisplay);
		CXStr value("Group by NPC");
		((CXWnd*)GroupedByNpcCheck)->SetWindowTextA(value);
	}
}

NativeAutoLootRow* NativeAutoLootWnd::GetSelectedRow()
{
	CListWnd* list = ActiveList ? ActiveList : PersonalList;
	NativeAutoLootRow* row = GetSelectedRowFromList(list);
	if (!row && list != SharedList) {
		row = GetSelectedRowFromList(SharedList);
	}

	return row;
}

NativeAutoLootRow* NativeAutoLootWnd::GetSelectedRowFromList(CListWnd* list)
{
	if (!list) {
		return nullptr;
	}

	const int selected = list->GetCurSel();
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

static void NativeChatTimestampConfigPath(char* path, size_t path_size)
{
	if (!path || !path_size) {
		return;
	}

	if (gszEQPath[0]) {
		sprintf_s(path, path_size, "%s\\native_interface.ini", gszEQPath);
	}
	else {
		strcpy_s(path, path_size, "native_interface.ini");
	}
}

static void NativeChatTimestampLoadConfig()
{
	if (gNativeChatTimestampConfigLoaded) {
		return;
	}

	char path[MAX_PATH];
	NativeChatTimestampConfigPath(path, sizeof(path));
	gNativeChatTimestampEnabled = GetPrivateProfileIntA("Chat", "Timestamps", 0, path) != 0;
	gNativeChatTimestampConfigLoaded = true;
}

static void NativeChatTimestampSaveConfig()
{
	char path[MAX_PATH];
	NativeChatTimestampConfigPath(path, sizeof(path));
	WritePrivateProfileStringA("Chat", "Timestamps", gNativeChatTimestampEnabled ? "1" : "0", path);
}

static void NativeChatTimestampStatus()
{
	NativeChatTimestampLoadConfig();
	nativeinterface::Chat(
		"Chat timestamps are %s. Use /timestamp on or /timestamp off.",
		gNativeChatTimestampEnabled ? "ON" : "OFF"
	);
}

static bool NativeChatTimestampHandleCommand(const char* line)
{
	if (!line) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/timestamp", &arguments) &&
		!NativeCommandMatch(line, "/timestamps", &arguments)
	) {
		return false;
	}

	NativeChatTimestampLoadConfig();

	if (!arguments || !arguments[0] || NativeCommandMatch(arguments, "toggle", nullptr)) {
		gNativeChatTimestampEnabled = !gNativeChatTimestampEnabled;
		NativeChatTimestampSaveConfig();
		NativeChatTimestampStatus();
		return true;
	}

	if (
		NativeCommandMatch(arguments, "on", nullptr) ||
		NativeCommandMatch(arguments, "1", nullptr) ||
		NativeCommandMatch(arguments, "true", nullptr)
	) {
		gNativeChatTimestampEnabled = true;
		NativeChatTimestampSaveConfig();
		NativeChatTimestampStatus();
		return true;
	}

	if (
		NativeCommandMatch(arguments, "off", nullptr) ||
		NativeCommandMatch(arguments, "0", nullptr) ||
		NativeCommandMatch(arguments, "false", nullptr)
	) {
		gNativeChatTimestampEnabled = false;
		NativeChatTimestampSaveConfig();
		NativeChatTimestampStatus();
		return true;
	}

	if (
		NativeCommandMatch(arguments, "status", nullptr) ||
		NativeCommandMatch(arguments, "show", nullptr)
	) {
		NativeChatTimestampStatus();
		return true;
	}

	nativeinterface::Chat("Usage: /timestamp [on|off|toggle|status]");
	return true;
}

static bool NativeChatTimestampFormatLine(const char* message, char* output, size_t output_size)
{
	if (!message || !message[0] || !output || output_size == 0) {
		return false;
	}

	NativeChatTimestampLoadConfig();
	if (!gNativeChatTimestampEnabled) {
		return false;
	}

	SYSTEMTIME now;
	GetLocalTime(&now);
	_snprintf_s(
		output,
		output_size,
		_TRUNCATE,
		"[%02u:%02u:%02u] %s",
		static_cast<unsigned>(now.wHour),
		static_cast<unsigned>(now.wMinute),
		static_cast<unsigned>(now.wSecond),
		message
	);

	return true;
}

static bool NativePetClassTokenMatch(const char* token, size_t token_length)
{
	if (!token || token_length == 0 || token_length >= 32) {
		return false;
	}

	char normalized[32];
	_snprintf_s(normalized, sizeof(normalized), _TRUNCATE, "%.*s", static_cast<int>(token_length), token);
	for (char *current = normalized; *current; ++current) {
		*current = static_cast<char>(std::tolower(static_cast<unsigned char>(*current)));
	}

	return !strcmp(normalized, "nec") ||
		!strcmp(normalized, "necro") ||
		!strcmp(normalized, "necromancer") ||
		!strcmp(normalized, "mag") ||
		!strcmp(normalized, "mage") ||
		!strcmp(normalized, "magician") ||
		!strcmp(normalized, "bst") ||
		!strcmp(normalized, "beast") ||
		!strcmp(normalized, "beastlord");
}

static bool NativePetClassRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (!NativeCommandMatch(line, "/pet", &arguments) || !arguments || !arguments[0]) {
		return false;
	}

	while (*arguments == ' ' || *arguments == '\t') {
		++arguments;
	}

	const char* token_end = arguments;
	while (*token_end && *token_end != ' ' && *token_end != '\t') {
		++token_end;
	}

	const auto token_length = static_cast<size_t>(token_end - arguments);
	if (!NativePetClassTokenMatch(arguments, token_length)) {
		return false;
	}

	while (*token_end == ' ' || *token_end == '\t') {
		++token_end;
	}

	if (*token_end) {
		_snprintf_s(output, output_size, _TRUNCATE, "/say #mc pet %.*s %s", static_cast<int>(token_length), arguments, token_end);
	}
	else {
		_snprintf_s(output, output_size, _TRUNCATE, "/say #mc pet %.*s", static_cast<int>(token_length), arguments);
	}

	return true;
}

static bool NativeTradeskillsLocalCommand(const char* line)
{
	if (!line) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/tradeskills", &arguments) &&
		!NativeCommandMatch(line, "/tradeskill", &arguments) &&
		!NativeCommandMatch(line, "/tradeskillui", &arguments) &&
		!NativeCommandMatch(line, "/makeallwindow", &arguments)
	) {
		return false;
	}

	if (
		!arguments ||
		!arguments[0] ||
		NativeCommandMatch(arguments, "open", nullptr) ||
		NativeCommandMatch(arguments, "window", nullptr) ||
		NativeCommandMatch(arguments, "ui", nullptr) ||
		NativeCommandMatch(arguments, "panel", nullptr)
	) {
		NativeTradeskillsEnsureWindow(true);
		if (gNativeTradeskillsWnd) {
			gNativeTradeskillsWnd->SetStatus("Select a learned recipe, then use Make All.");
		}
		return true;
	}

	return false;
}

static bool NativeTradeskillsRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (NativeCommandMatch(line, "/makeall", &arguments)) {
		if (!arguments || !arguments[0]) {
			strcpy_s(output, output_size, "/say #ts makeall");
		}
		else {
			_snprintf_s(output, output_size, _TRUNCATE, "/say #ts makeall %s", arguments);
		}
		return true;
	}

	if (
		!NativeCommandMatch(line, "/tradeskills", &arguments) &&
		!NativeCommandMatch(line, "/tradeskill", &arguments) &&
		!NativeCommandMatch(line, "/ts", &arguments)
	) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		return false;
	}

	const char* make_all_arguments = nullptr;
	if (
		NativeCommandMatch(arguments, "makeall", &make_all_arguments) ||
		NativeCommandMatch(arguments, "all", &make_all_arguments)
	) {
		if (!make_all_arguments || !make_all_arguments[0]) {
			strcpy_s(output, output_size, "/say #ts makeall");
		}
		else {
			_snprintf_s(output, output_size, _TRUNCATE, "/say #ts makeall %s", make_all_arguments);
		}
		return true;
	}

	_snprintf_s(output, output_size, _TRUNCATE, "/say #ts %s", arguments);
	return true;
}

static bool NativeUIShowcaseHandleCommand(const char* line)
{
	if (!line) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/nativeui", &arguments) &&
		!NativeCommandMatch(line, "/uishowcase", &arguments) &&
		!NativeCommandMatch(line, "/showcase", &arguments)
	) {
		return false;
	}

	if (arguments && NativeCommandMatch(arguments, "close", nullptr)) {
		if (gNativeUIShowcaseWnd) {
			gNativeUIShowcaseWnd->pXWnd()->Show(0, 1);
		}
		return true;
	}

	NativeUIShowcaseEnsureWindow(true);
	if (gNativeUIShowcaseWnd) {
		if (arguments && NativeCommandMatch(arguments, "reset", nullptr)) {
			gNativeUIShowcaseWnd->ResetDemo();
			gNativeUIShowcaseWnd->SetStatus("Showcase opened and reset.");
		}
	}

	return true;
}

static bool NativeMulticlassRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		NativeCommandMatch(line, "/mc", &arguments) ||
		NativeCommandMatch(line, "/multiclass", &arguments) ||
		NativeCommandMatch(line, "/multiclassui", &arguments)
	) {
		if (!arguments || !arguments[0]) {
			strcpy_s(output, output_size, "/say #mc open");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "open", nullptr) ||
			NativeCommandMatch(arguments, "window", nullptr) ||
			NativeCommandMatch(arguments, "ui", nullptr) ||
			NativeCommandMatch(arguments, "panel", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc open");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "refresh", nullptr) ||
			NativeCommandMatch(arguments, "status", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc refresh");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "pets", nullptr) ||
			NativeCommandMatch(arguments, "petui", nullptr) ||
			NativeCommandMatch(arguments, "petwindow", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc pets");
			return true;
		}

		const char* disc_arguments = nullptr;
		if (
			NativeCommandMatch(arguments, "disc", &disc_arguments) ||
			NativeCommandMatch(arguments, "discs", &disc_arguments) ||
			NativeCommandMatch(arguments, "discipline", &disc_arguments) ||
			NativeCommandMatch(arguments, "disciplines", &disc_arguments)
		) {
			if (!disc_arguments || !disc_arguments[0]) {
				strcpy_s(output, output_size, "/say #mc disc open");
			}
			else {
				sprintf_s(output, output_size, "/say #mc disc %s", disc_arguments);
			}
			return true;
		}

		const char* melody_arguments = nullptr;
		if (
			NativeCommandMatch(arguments, "melody", &melody_arguments) ||
			NativeCommandMatch(arguments, "bardmelody", &melody_arguments) ||
			NativeCommandMatch(arguments, "songui", &melody_arguments)
		) {
			if (!melody_arguments || !melody_arguments[0]) {
				strcpy_s(output, output_size, "/say #mc melody open");
			}
			else {
				sprintf_s(output, output_size, "/say #mc melody %s", melody_arguments);
			}
			return true;
		}

		sprintf_s(output, output_size, "/say #mc %s", arguments);
		return true;
	}

	if (
		NativeCommandMatch(line, "/mcpets", &arguments) ||
		NativeCommandMatch(line, "/multiclasspets", &arguments) ||
		NativeCommandMatch(line, "/petui", &arguments)
	) {
		if (!arguments || !arguments[0]) {
			strcpy_s(output, output_size, "/say #mc pets");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "open", nullptr) ||
			NativeCommandMatch(arguments, "window", nullptr) ||
			NativeCommandMatch(arguments, "ui", nullptr) ||
			NativeCommandMatch(arguments, "panel", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc pets");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "refresh", nullptr) ||
			NativeCommandMatch(arguments, "status", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc pet refresh");
			return true;
		}

		sprintf_s(output, output_size, "/say #mc pet %s", arguments);
		return true;
	}

	if (
		NativeCommandMatch(line, "/disc", &arguments) ||
		NativeCommandMatch(line, "/discs", &arguments) ||
		NativeCommandMatch(line, "/discipline", &arguments) ||
		NativeCommandMatch(line, "/disciplines", &arguments) ||
		NativeCommandMatch(line, "/discwindow", &arguments) ||
		NativeCommandMatch(line, "/combatability", &arguments) ||
		NativeCommandMatch(line, "/combatabilities", &arguments) ||
		NativeCommandMatch(line, "/combatdisc", &arguments)
	) {
		if (!arguments || !arguments[0]) {
			strcpy_s(output, output_size, "/say #mc disc open");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "open", nullptr) ||
			NativeCommandMatch(arguments, "window", nullptr) ||
			NativeCommandMatch(arguments, "ui", nullptr) ||
			NativeCommandMatch(arguments, "panel", nullptr) ||
			NativeCommandMatch(arguments, "list", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc disc open");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "refresh", nullptr) ||
			NativeCommandMatch(arguments, "status", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc disc refresh");
			return true;
		}

		sprintf_s(output, output_size, "/say #mc disc %s", arguments);
		return true;
	}

	if (
		NativeCommandMatch(line, "/mcmelody", &arguments) ||
		NativeCommandMatch(line, "/multiclassmelody", &arguments) ||
		NativeCommandMatch(line, "/melodyui", &arguments)
	) {
		if (!arguments || !arguments[0]) {
			strcpy_s(output, output_size, "/say #mc melody open");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "open", nullptr) ||
			NativeCommandMatch(arguments, "window", nullptr) ||
			NativeCommandMatch(arguments, "ui", nullptr) ||
			NativeCommandMatch(arguments, "panel", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc melody open");
			return true;
		}

		if (
			NativeCommandMatch(arguments, "refresh", nullptr) ||
			NativeCommandMatch(arguments, "status", nullptr)
		) {
			strcpy_s(output, output_size, "/say #mc melody refresh");
			return true;
		}

		sprintf_s(output, output_size, "/say #mc melody %s", arguments);
		return true;
	}

	return false;
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

static bool NativeFactionRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/rep", &arguments) &&
		!NativeCommandMatch(line, "/reputation", &arguments) &&
		!NativeCommandMatch(line, "/factionwindow", &arguments) &&
		!NativeCommandMatch(line, "/factionstatus", &arguments)
	) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		strcpy_s(output, output_size, "/say #rep refresh");
		return true;
	}

	if (
		NativeCommandMatch(arguments, "open", nullptr) ||
		NativeCommandMatch(arguments, "window", nullptr) ||
		NativeCommandMatch(arguments, "ui", nullptr) ||
		NativeCommandMatch(arguments, "panel", nullptr) ||
		NativeCommandMatch(arguments, "refresh", nullptr) ||
		NativeCommandMatch(arguments, "status", nullptr)
	) {
		strcpy_s(output, output_size, "/say #rep refresh");
		return true;
	}

	sprintf_s(output, output_size, "/say #rep %s", arguments);
	return true;
}

static bool NativeDpsRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/dps", &arguments) &&
		!NativeCommandMatch(line, "/dpsparser", &arguments) &&
		!NativeCommandMatch(line, "/damageparser", &arguments)
	) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		strcpy_s(output, output_size, "/say #dps live on");
		return true;
	}

	if (
		NativeCommandMatch(arguments, "open", nullptr) ||
		NativeCommandMatch(arguments, "window", nullptr) ||
		NativeCommandMatch(arguments, "ui", nullptr) ||
		NativeCommandMatch(arguments, "refresh", nullptr)
	) {
		strcpy_s(output, output_size, "/say #dps live on");
		return true;
	}

	sprintf_s(output, output_size, "/say #dps %s", arguments);
	return true;
}

static bool NativeUseItemRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (!NativeCommandMatch(line, "/useitem", &arguments)) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		strcpy_s(output, output_size, "/say #useitem");
		return true;
	}

	sprintf_s(output, output_size, "/say #useitem %s", arguments);
	return true;
}

static bool NativeAAExpRewriteCommand(const char* line, char* output, size_t output_size)
{
	if (!line || !output || !output_size) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/aaexp", &arguments) &&
		!NativeCommandMatch(line, "/aaxp", &arguments)
	) {
		return false;
	}

	if (!arguments || !arguments[0]) {
		strcpy_s(output, output_size, "/say #aaexp status");
		return true;
	}

	sprintf_s(output, output_size, "/say #aaexp %s", arguments);
	return true;
}

static bool gNativeAutoFollowEnabled = false;
static DWORD gNativeAutoFollowTargetId = 0;
static float gNativeAutoFollowDistance = 20.0f;
static float gNativeAutoFollowLastDistance = 0.0f;
static int gNativeAutoFollowStuckPulses = 0;

static PSPAWNINFO NativeAutoFollowTarget()
{
	if (gNativeAutoFollowTargetId) {
		return (PSPAWNINFO)GetSpawnByID(gNativeAutoFollowTargetId);
	}

	return pTarget ? (PSPAWNINFO)pTarget : nullptr;
}

static void NativeAutoFollowStop(const char* reason)
{
	gNativeAutoFollowEnabled = false;
	gNativeAutoFollowTargetId = 0;
	gNativeAutoFollowStuckPulses = 0;
	if (pCharSpawn) {
		((PSPAWNINFO)pCharSpawn)->WhoFollowing = nullptr;
	}
	NativeAutoLootSendCommand("/follow off");
	if (reason && reason[0]) {
		nativeinterface::Chat("AutoFollow: %s", reason);
	}
}

static bool NativeAutoFollowLocalCommand(const char* line)
{
	if (!line) {
		return false;
	}

	while (*line == ' ' || *line == '\t') {
		++line;
	}

	const char* arguments = nullptr;
	if (
		!NativeCommandMatch(line, "/afollow", &arguments) &&
		!NativeCommandMatch(line, "/autofollow", &arguments)
	) {
		return false;
	}

	if (arguments && NativeCommandMatch(arguments, "off", nullptr)) {
		NativeAutoFollowStop("off");
		return true;
	}

	if (arguments && NativeCommandMatch(arguments, "status", nullptr)) {
		nativeinterface::Chat(
			"AutoFollow: %s, distance %.0f",
			gNativeAutoFollowEnabled ? "on" : "off",
			gNativeAutoFollowDistance
		);
		return true;
	}

	const char* distance_args = nullptr;
	if (arguments && NativeCommandMatch(arguments, "distance", &distance_args)) {
		const float distance = distance_args && distance_args[0] ? static_cast<float>(atof(distance_args)) : 0.0f;
		if (distance >= 8.0f && distance <= 80.0f) {
			gNativeAutoFollowDistance = distance;
			nativeinterface::Chat("AutoFollow: distance set to %.0f", gNativeAutoFollowDistance);
		} else {
			nativeinterface::Chat("AutoFollow: distance must be 8 to 80.");
		}
		return true;
	}

	if (!pCharSpawn || !pTarget || pTarget == pCharSpawn) {
		nativeinterface::Chat("AutoFollow: target a player or NPC first.");
		return true;
	}

	PSPAWNINFO target = (PSPAWNINFO)pTarget;
	gNativeAutoFollowEnabled = true;
	gNativeAutoFollowTargetId = target->SpawnID;
	gNativeAutoFollowLastDistance = DistanceToSpawn((PSPAWNINFO)pCharSpawn, target);
	gNativeAutoFollowStuckPulses = 0;
	NativeAutoLootSendCommand("/follow");
	nativeinterface::Chat("AutoFollow: following %s at %.0f.", target->DisplayedName[0] ? target->DisplayedName : target->Name, gNativeAutoFollowDistance);
	return true;
}

static void NativeAutoFollowPulse()
{
	if (!gNativeAutoFollowEnabled || !pCharSpawn || !pLocalPlayer) {
		return;
	}

	PSPAWNINFO target = NativeAutoFollowTarget();
	if (!target) {
		NativeAutoFollowStop("target lost");
		return;
	}

	PSPAWNINFO self = (PSPAWNINFO)pCharSpawn;
	const float distance = DistanceToSpawn(self, target);
	if (distance > 250.0f) {
		NativeAutoFollowStop("target too far");
		return;
	}

	if (distance <= gNativeAutoFollowDistance) {
		self->WhoFollowing = nullptr;
		gNativeAutoFollowStuckPulses = 0;
		gNativeAutoFollowLastDistance = distance;
		return;
	}

	if (!self->WhoFollowing) {
		if (!pTarget || ((PSPAWNINFO)pTarget)->SpawnID != gNativeAutoFollowTargetId) {
			NativeAutoFollowStop("target no longer selected");
			return;
		}
		NativeAutoLootSendCommand("/follow");
	}

	if (distance >= gNativeAutoFollowLastDistance - 1.0f) {
		++gNativeAutoFollowStuckPulses;
	} else {
		gNativeAutoFollowStuckPulses = 0;
	}
	gNativeAutoFollowLastDistance = distance;

	if (gNativeAutoFollowStuckPulses > 120) {
		NativeAutoFollowStop("stuck");
	}
}

class NativeAutoLootCommandHook
{
public:
	VOID Trampoline(EQPlayer* player, PCHAR line);
	VOID Detour(EQPlayer* player, PCHAR line)
	{
		if (NativeUIShowcaseHandleCommand(line)) {
			NativeAutoLootTrace("handled local UI showcase command: %s", line ? line : "");
			return;
		}

		if (NativeChatTimestampHandleCommand(line)) {
			NativeAutoLootTrace("handled chat timestamp command: %s", line ? line : "");
			return;
		}

		if (NativeTradeskillsLocalCommand(line)) {
			NativeAutoLootTrace("handled local tradeskills command: %s", line ? line : "");
			return;
		}

		if (NativeAutoFollowLocalCommand(line)) {
			NativeAutoLootTrace("handled local autofollow command: %s", line ? line : "");
			return;
		}

		char rewritten[256];

		if (NativeTradeskillsRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativePetClassRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativeMulticlassRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativeAchievementRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativeFactionRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativeDpsRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativeUseItemRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (NativeAAExpRewriteCommand(line, rewritten, sizeof(rewritten))) {
			NativeAutoLootTrace("rewrite command: %s -> %s", line ? line : "", rewritten);
			Trampoline(player, rewritten);
			return;
		}

		if (nativeinterface::HandleCommandLine(line)) {
			NativeAutoLootTrace("handled native interface command: %s", line ? line : "");
			return;
		}

		Trampoline(player, line);
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID NativeAutoLootCommandHook::Trampoline(EQPlayer* player, PCHAR line));

static void NativeAutoLootUpdateWindow()
{
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

	if (gNativeAutoLootWnd) {
		gNativeAutoLootWnd->RefreshRows();
		char status[128];
		sprintf_s(status, "Personal %d / Shared %d", personal, shared);
		gNativeAutoLootWnd->SetStatus(status);
	}

	if (gNativeAutoLootSettingsWnd) {
		gNativeAutoLootSettingsWnd->RefreshRows();
	}
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

static bool NativeMulticlassParseTransport(const char* message)
{
	if (!message || !message[0] || !NativeStartsWith(message, "MULTICLASS|")) {
		return false;
	}

	if (NativeStartsWith(message, "MULTICLASS|window|clear")) {
		gNativeMulticlassPets.clear();
		gNativeMulticlassMelodySlots.clear();
		gNativeMulticlassMelodySongs.clear();
		gNativeMulticlassDisciplineRows.clear();
		gNativeMulticlassState.selection_status = "Refreshing Multiclass profile...";
		gNativeMulticlassState.skill_summary = "Skills: refreshing...";
		gNativeMulticlassState.bonus_summary = "Resonance bonuses: refreshing...";
		gNativeMulticlassState.melody_status = "Bard Melody: refreshing...";
		gNativeMulticlassState.discipline_status = "Disciplines: refreshing...";
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|profile|")) {
		const std::string payload(message + strlen("MULTICLASS|profile|"));
		gNativeMulticlassState.has_profile = true;
		gNativeMulticlassState.profile_name = NativeGetPairValue(payload, "name");
		gNativeMulticlassState.resonance_key = NativeGetPairValue(payload, "resonance");
		gNativeMulticlassState.class1 = NativeToInt(NativeGetPairValue(payload, "class1"));
		gNativeMulticlassState.class2 = NativeToInt(NativeGetPairValue(payload, "class2"));
		gNativeMulticlassState.class3 = NativeToInt(NativeGetPairValue(payload, "class3"));
		gNativeMulticlassState.presentation = NativeToInt(NativeGetPairValue(payload, "presentation"));
		gNativeMulticlassState.base = NativeToInt(NativeGetPairValue(payload, "base"));
		gNativeMulticlassState.multiple_pets = NativeToBool(NativeGetPairValue(payload, "pets"));
		gNativeMulticlassState.locked = NativeToBool(NativeGetPairValue(payload, "locked"));
		gNativeMulticlassState.reweaves = NativeToInt(NativeGetPairValue(payload, "reweaves"));
		gNativeMulticlassState.class1_name = NativeGetPairValue(payload, "class1_name");
		gNativeMulticlassState.class2_name = NativeGetPairValue(payload, "class2_name");
		gNativeMulticlassState.class3_name = NativeGetPairValue(payload, "class3_name");
		gNativeMulticlassState.presentation_name = NativeGetPairValue(payload, "presentation_name");
		gNativeMulticlassState.base_name = NativeGetPairValue(payload, "base_name");

		if (gNativeMulticlassState.profile_name.empty()) {
			gNativeMulticlassState.profile_name = "Multiclass Trio";
		}
		if (gNativeMulticlassState.class1_name.empty()) {
			gNativeMulticlassState.class1_name = NativeMulticlassClassName(gNativeMulticlassState.class1);
		}
		if (gNativeMulticlassState.class2_name.empty()) {
			gNativeMulticlassState.class2_name = NativeMulticlassClassName(gNativeMulticlassState.class2);
		}
		if (gNativeMulticlassState.class3_name.empty()) {
			gNativeMulticlassState.class3_name = NativeMulticlassClassName(gNativeMulticlassState.class3);
		}
		if (gNativeMulticlassState.presentation_name.empty()) {
			gNativeMulticlassState.presentation_name = NativeMulticlassClassName(gNativeMulticlassState.presentation);
		}
		if (gNativeMulticlassState.base_name.empty()) {
			gNativeMulticlassState.base_name = NativeMulticlassClassName(gNativeMulticlassState.base);
		}

		if (NativeMulticlassIsPlayerClass(gNativeMulticlassState.class2)) {
			gNativeMulticlassState.selected_slot2 = gNativeMulticlassState.class2;
		}
		if (NativeMulticlassIsPlayerClass(gNativeMulticlassState.class3)) {
			gNativeMulticlassState.selected_slot3 = gNativeMulticlassState.class3;
		}

		NativeMulticlassNormalizeSelections();
		NativeMulticlassScheduleCasterUI();
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|vitals|")) {
		const std::string payload(message + strlen("MULTICLASS|vitals|"));
		gNativeMulticlassState.mana = NativeToInt(NativeGetPairValue(payload, "mana"));
		gNativeMulticlassState.max_mana = NativeToInt(NativeGetPairValue(payload, "max_mana"));
		gNativeMulticlassState.endurance = NativeToInt(NativeGetPairValue(payload, "endurance"));
		gNativeMulticlassState.max_endurance = NativeToInt(NativeGetPairValue(payload, "max_endurance"));
		gNativeMulticlassState.class_mask = NativeToInt(NativeGetPairValue(payload, "class_mask"));
		gNativeMulticlassState.aa_mask = NativeToInt(NativeGetPairValue(payload, "aa_mask"));
		const int presentation = NativeToInt(NativeGetPairValue(payload, "presentation"));
		const int base = NativeToInt(NativeGetPairValue(payload, "base"));
		if (NativeMulticlassIsPlayerClass(presentation)) {
			gNativeMulticlassState.presentation = presentation;
			gNativeMulticlassState.presentation_name = NativeMulticlassClassName(presentation);
		}
		if (NativeMulticlassIsPlayerClass(base)) {
			gNativeMulticlassState.base = base;
			gNativeMulticlassState.base_name = NativeMulticlassClassName(base);
		}
		NativeMulticlassScheduleCasterUI();
		NativeMulticlassPatchLocalVitals();
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|trio|")) {
		const std::string payload(message + strlen("MULTICLASS|trio|"));
		gNativeMulticlassState.roles = NativeGetPairValue(payload, "roles");
		gNativeMulticlassState.resonance = NativeGetPairValue(payload, "resonance");
		gNativeMulticlassState.summary = NativeGetPairValue(payload, "summary");
		if (gNativeMulticlassState.roles.empty()) {
			gNativeMulticlassState.roles = "-";
		}
		if (gNativeMulticlassState.resonance.empty()) {
			gNativeMulticlassState.resonance = "Trio Notes";
		}
		if (gNativeMulticlassState.summary.empty()) {
			gNativeMulticlassState.summary = "Fixed trio identity.";
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|skills|")) {
		const std::string payload(message + strlen("MULTICLASS|skills|"));
		gNativeMulticlassState.skill_summary = NativeGetPairValue(payload, "summary");
		if (gNativeMulticlassState.skill_summary.empty()) {
			gNativeMulticlassState.skill_summary = "Skills: waiting for profile.";
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|bonuses|")) {
		const std::string payload(message + strlen("MULTICLASS|bonuses|"));
		gNativeMulticlassState.bonus_summary = NativeGetPairValue(payload, "summary");
		if (gNativeMulticlassState.bonus_summary.empty()) {
			gNativeMulticlassState.bonus_summary = "Resonance bonuses: waiting for profile.";
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|selection|")) {
		const std::string payload(message + strlen("MULTICLASS|selection|"));
		gNativeMulticlassState.can_choose = NativeToBool(NativeGetPairValue(payload, "can_choose"));
		gNativeMulticlassState.selection_status = NativeGetPairValue(payload, "status");
		if (gNativeMulticlassState.selection_status.empty()) {
			gNativeMulticlassState.selection_status = gNativeMulticlassState.locked ? "Trio locked." : "Choose two added classes.";
		}
		NativeMulticlassNormalizeSelections();
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|spell_levels|begin")) {
		gNativeMulticlassSpellLevelsLoading = true;
		gNativeMulticlassSpellLevelPatchCount = 0;
		gNativeMulticlassSpellLevelReapplyDelay = 0;
		gNativeMulticlassSpellLevelReapplyPasses = 0;
		gNativeMulticlassSpellLevelsById.clear();
		gNativeMulticlassSpellLevelsByName.clear();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|spell_level|")) {
		return NativeMulticlassApplySpellLevelPatch(std::string(message + strlen("MULTICLASS|spell_level|")));
	}

	if (NativeStartsWith(message, "MULTICLASS|spell_levels|end")) {
		const std::string payload(message + strlen("MULTICLASS|spell_levels|end"));
		const int expected_count = NativeToInt(NativeGetPairValue(payload, "count"), gNativeMulticlassSpellLevelPatchCount);
		gNativeMulticlassSpellLevelsLoading = false;
		const int applied_count = NativeMulticlassApplyCachedSpellLevels();
		NativeMulticlassScheduleSpellLevelReapply();
		NativeAutoLootTrace("Multiclass spellbook levels cached: %d/%d, applied: %d", gNativeMulticlassSpellLevelPatchCount, expected_count, applied_count);
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|pet_roster|")) {
		const std::string payload(message + strlen("MULTICLASS|pet_roster|"));
		gNativeMulticlassPets.clear();
		gNativeMulticlassState.roster_count = NativeToInt(NativeGetPairValue(payload, "count"));
		gNativeMulticlassState.roster_limit = NativeToInt(NativeGetPairValue(payload, "limit"), 1);
		gNativeMulticlassState.focus_id = NativeToInt(NativeGetPairValue(payload, "focus"));
		gNativeMulticlassState.pet_policy = NativeGetPairValue(payload, "policy");
		gNativeMulticlassState.pet_control = NativeGetPairValue(payload, "control");
		if (gNativeMulticlassState.pet_policy.empty()) {
			gNativeMulticlassState.pet_policy = "single-pet";
		}
		if (gNativeMulticlassState.pet_control.empty()) {
			gNativeMulticlassState.pet_control = "Attack/Back/Follow/Guard affect all pets; toggles affect focus.";
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|pet|")) {
		const std::string payload(message + strlen("MULTICLASS|pet|"));
		NativeMulticlassPetRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.name = NativeGetPairValue(payload, "name");
		row.hp = NativeToInt(NativeGetPairValue(payload, "hp"));
		row.mana = NativeToInt(NativeGetPairValue(payload, "mana"));
		row.target = NativeGetPairValue(payload, "target");
		row.taunt = NativeToBool(NativeGetPairValue(payload, "taunt"));
		row.hold = NativeToBool(NativeGetPairValue(payload, "hold"));
		row.spellhold = NativeToBool(NativeGetPairValue(payload, "spellhold"));
		row.order = NativeGetPairValue(payload, "order");
		row.focused = NativeToBool(NativeGetPairValue(payload, "focused"));
		if (row.name.empty()) {
			row.name = "Pet";
		}
		if (row.target.empty()) {
			row.target = "-";
		}
		if (row.order.empty()) {
			row.order = "follow";
		}
		if (row.focused) {
			gNativeMulticlassState.focus_id = row.id;
		}
		if (row.id > 0) {
			gNativeMulticlassPets.push_back(row);
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|melody|")) {
		const std::string payload(message + strlen("MULTICLASS|melody|"));
		gNativeMulticlassState.has_bard = NativeToBool(NativeGetPairValue(payload, "has_bard"));
		gNativeMulticlassState.melody_status = NativeGetPairValue(payload, "status");
		if (gNativeMulticlassState.melody_status.empty()) {
			gNativeMulticlassState.melody_status = gNativeMulticlassState.has_bard ? "Bard Melody: ready." : "Bard Melody: Bard not in trio.";
		}
		gNativeMulticlassMelodySlots.clear();
		gNativeMulticlassMelodySongs.clear();
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|melody_slot|")) {
		const std::string payload(message + strlen("MULTICLASS|melody_slot|"));
		NativeMulticlassMelodySlot row;
		row.slot = NativeToInt(NativeGetPairValue(payload, "slot"));
		row.spell_id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.name = NativeGetPairValue(payload, "name");
		row.level = NativeToInt(NativeGetPairValue(payload, "level"));
		row.state = NativeGetPairValue(payload, "state");
		if (row.name.empty()) {
			row.name = "-";
		}
		if (row.state.empty()) {
			row.state = row.spell_id > 0 ? "selected" : "empty";
		}
		if (row.slot >= 1 && row.slot <= 4) {
			gNativeMulticlassMelodySlots.push_back(row);
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|melody_song|")) {
		const std::string payload(message + strlen("MULTICLASS|melody_song|"));
		NativeMulticlassMelodySong row;
		row.spell_id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.name = NativeGetPairValue(payload, "name");
		row.level = NativeToInt(NativeGetPairValue(payload, "level"));
		row.allowed = NativeToBool(NativeGetPairValue(payload, "allowed"));
		row.reason = NativeGetPairValue(payload, "reason");
		if (row.name.empty()) {
			row.name = "Song";
		}
		if (row.spell_id > 0) {
			gNativeMulticlassMelodySongs.push_back(row);
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|melody_songs|end")) {
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|disciplines|clear")) {
		gNativeMulticlassDisciplineRows.clear();
		gNativeMulticlassState.discipline_status = "Disciplines: refreshing...";
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|disciplines|summary|")) {
		const std::string payload(message + strlen("MULTICLASS|disciplines|summary|"));
		gNativeMulticlassState.discipline_status = NativeGetPairValue(payload, "status");
		if (gNativeMulticlassState.discipline_status.empty()) {
			gNativeMulticlassState.discipline_status = "Disciplines: ready.";
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|discipline|")) {
		const std::string payload(message + strlen("MULTICLASS|discipline|"));
		NativeMulticlassDisciplineRow row;
		row.slot = NativeToInt(NativeGetPairValue(payload, "slot"));
		row.spell_id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.name = NativeGetPairValue(payload, "name");
		row.level = NativeToInt(NativeGetPairValue(payload, "level"));
		row.timer = NativeToInt(NativeGetPairValue(payload, "timer"));
		row.timer_total = NativeToInt(NativeGetPairValue(payload, "total"));
		if (row.timer_total <= 0) {
			row.timer_total = row.timer;
		}
		row.timer_received_ms = GetTickCount();
		row.ready = NativeToBool(NativeGetPairValue(payload, "ready"));
		row.state = NativeGetPairValue(payload, "state");
		if (row.name.empty()) {
			row.name = "Discipline";
		}
		if (row.state.empty()) {
			row.state = row.ready ? "Ready" : "Blocked";
		}
		if (row.spell_id > 0) {
			gNativeMulticlassDisciplineRows.push_back(row);
		}
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|disciplines|end")) {
		NativeMulticlassRefreshWindows();
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|window|show")) {
		NativeMulticlassEnsureWindow(true);
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|pet_window|show")) {
		NativeMulticlassEnsurePetWindow(true);
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|melody_window|show")) {
		NativeMulticlassEnsureMelodyWindow(true);
		return true;
	}

	if (NativeStartsWith(message, "MULTICLASS|discipline_window|show")) {
		NativeMulticlassEnsureDisciplineWindow(true);
		return true;
	}

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
		gNativeAchievementRewards.clear();
		gNativeAchievementSelectedCategory = 0;
		gNativeAchievementSelectedAchievement = 0;
		gNativeAchievementCategoriesDirty = true;
		gNativeAchievementRowsDirty = true;
		gNativeAchievementObjectivesDirty = true;
		gNativeAchievementRewardsDirty = true;
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
		gNativeAchievementRewards.clear();
		gNativeAchievementSelectedAchievement = 0;
		gNativeAchievementRowsDirty = true;
		gNativeAchievementObjectivesDirty = true;
		gNativeAchievementRewardsDirty = true;
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

	if (NativeStartsWith(message, "ACH|rewards|clear")) {
		gNativeAchievementRewards.clear();
		gNativeAchievementRewardsDirty = true;
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
		gNativeAchievementRewards.clear();
		gNativeAchievementRewardsDirty = true;
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

	if (NativeStartsWith(message, "ACH|reward|")) {
		const std::string payload(message + strlen("ACH|reward|"));
		NativeAchievementRewardRow row;
		row.definition_id = static_cast<uint64_t>(_strtoui64(NativeGetPairValue(payload, "definition").c_str(), nullptr, 10));
		row.type = NativeGetPairValue(payload, "type");
		row.reward_id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.amount = NativeToInt(NativeGetPairValue(payload, "amount"));
		row.auto_claim = NativeToBool(NativeGetPairValue(payload, "auto"));
		row.tier = NativeGetPairValue(payload, "tier");
		row.name = NativeGetPairValue(payload, "name");
		if (row.definition_id > 0 || !row.name.empty()) {
			gNativeAchievementRewards.push_back(row);
			gNativeAchievementRewardsDirty = true;
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

static bool NativeFactionParseTransport(const char* message)
{
	if (!message || !message[0] || !NativeStartsWith(message, "FACTION|")) {
		return false;
	}

	if (NativeStartsWith(message, "FACTION|window|clear")) {
		gNativeFactionLoading = true;
		gNativeFactionRows.clear();
		gNativeFactionTargetId = 0;
		gNativeFactionMode.clear();
		gNativeFactionStatus = "Loading faction reputation...";
		gNativeFactionRowsDirty = true;
		if (gNativeFactionWnd) {
			gNativeFactionWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "FACTION|summary|")) {
		const std::string payload(message + strlen("FACTION|summary|"));
		gNativeFactionTargetId = NativeToInt(NativeGetPairValue(payload, "target"));
		gNativeFactionMode = NativeGetPairValue(payload, "mode");
		const std::string server_search = NativeGetPairValue(payload, "search");
		if (!server_search.empty() || gNativeFactionMode == "search") {
			gNativeFactionSearch = server_search;
		}
		gNativeFactionStatus = NativeGetPairValue(payload, "status");
		if (gNativeFactionStatus.empty()) {
			gNativeFactionStatus = "Faction reputation refreshed.";
		}
		if (gNativeFactionWnd && !gNativeFactionLoading) {
			gNativeFactionWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "FACTION|row|")) {
		const std::string payload(message + strlen("FACTION|row|"));
		NativeFactionRow row;
		row.id = NativeToInt(NativeGetPairValue(payload, "id"));
		row.name = NativeGetPairValue(payload, "name");
		row.raw_value = NativeToInt(NativeGetPairValue(payload, "raw"));
		row.modified_value = NativeToInt(NativeGetPairValue(payload, "mod"));
		row.standing = NativeGetPairValue(payload, "standing");
		row.touched = NativeToBool(NativeGetPairValue(payload, "touched"));
		row.target = NativeToBool(NativeGetPairValue(payload, "target"));
		row.pinned = NativeToBool(NativeGetPairValue(payload, "pinned"));
		row.hidden = NativeToBool(NativeGetPairValue(payload, "hidden"));
		row.section = NativeGetPairValue(payload, "section");
		if (row.name.empty()) {
			char name[48];
			sprintf_s(name, "Faction %d", row.id);
			row.name = name;
		}
		if (row.standing.empty()) {
			row.standing = "Unknown";
		}
		if (row.id > 0) {
			gNativeFactionRows.push_back(row);
			gNativeFactionRowsDirty = true;
		}
		if (gNativeFactionWnd && !gNativeFactionLoading) {
			gNativeFactionWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "FACTION|window|show")) {
		gNativeFactionLoading = false;
		NativeFactionEnsureWindow(true);
		if (gNativeFactionWnd) {
			gNativeFactionWnd->RefreshRows();
			gNativeFactionWnd->SetStatus(gNativeFactionStatus.c_str());
		}
		return true;
	}

	return true;
}

static bool NativeDpsParseTransport(const char* message)
{
	if (!message || !message[0] || !NativeStartsWith(message, "DPS|")) {
		return false;
	}

	if (NativeStartsWith(message, "DPS|window|clear")) {
		gNativeDpsLoading = true;
		gNativeDpsRows.clear();
		gNativeDpsRowsDirty = true;
		gNativeDpsStatus = "Loading DPS parser...";
		if (gNativeDpsWnd) {
			gNativeDpsWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "DPS|summary|")) {
		const std::string payload(message + strlen("DPS|summary|"));
		gNativeDpsEncounterId = NativeToInt(NativeGetPairValue(payload, "id"));
		gNativeDpsTarget = NativeGetPairValue(payload, "target");
		gNativeDpsElapsedMs = NativeToInt(NativeGetPairValue(payload, "elapsed"));
		gNativeDpsDamage = _strtoui64(NativeGetPairValue(payload, "damage").c_str(), nullptr, 10);
		gNativeDpsHealing = _strtoui64(NativeGetPairValue(payload, "healing").c_str(), nullptr, 10);
		gNativeDpsIncoming = _strtoui64(NativeGetPairValue(payload, "incoming").c_str(), nullptr, 10);
		gNativeDpsStatus = NativeGetPairValue(payload, "status");
		if (gNativeDpsStatus.empty()) {
			gNativeDpsStatus = "DPS parser refreshed.";
		}
		if (gNativeDpsWnd && !gNativeDpsLoading) {
			gNativeDpsWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "DPS|row|")) {
		const std::string payload(message + strlen("DPS|row|"));
		NativeDpsRow row;
		row.actor_id = NativeToInt(NativeGetPairValue(payload, "actor_id"));
		row.owner_id = NativeToInt(NativeGetPairValue(payload, "owner_id"));
		row.actor = NativeGetPairValue(payload, "actor");
		row.source = NativeGetPairValue(payload, "source");
		row.damage = _strtoui64(NativeGetPairValue(payload, "damage").c_str(), nullptr, 10);
		row.healing = _strtoui64(NativeGetPairValue(payload, "healing").c_str(), nullptr, 10);
		row.incoming = _strtoui64(NativeGetPairValue(payload, "incoming").c_str(), nullptr, 10);
		row.dps = _strtoui64(NativeGetPairValue(payload, "dps").c_str(), nullptr, 10);
		row.hps = _strtoui64(NativeGetPairValue(payload, "hps").c_str(), nullptr, 10);
		row.pct = NativeToInt(NativeGetPairValue(payload, "pct"));
		if (row.actor.empty()) {
			row.actor = "Actor";
		}
		if (row.source.empty()) {
			row.source = row.actor;
		}
		gNativeDpsRows.push_back(row);
		gNativeDpsRowsDirty = true;
		if (gNativeDpsWnd && !gNativeDpsLoading) {
			gNativeDpsWnd->RefreshRows();
		}
		return true;
	}

	if (NativeStartsWith(message, "DPS|window|end")) {
		gNativeDpsLoading = false;
		if (gNativeDpsWnd) {
			gNativeDpsWnd->RefreshRows();
			gNativeDpsWnd->SetStatus(gNativeDpsStatus.c_str());
		}
		return true;
	}

	if (NativeStartsWith(message, "DPS|window|show")) {
		gNativeDpsLoading = false;
		NativeDpsEnsureWindow(true);
		if (gNativeDpsWnd) {
			gNativeDpsWnd->RefreshRows();
			gNativeDpsWnd->SetStatus(gNativeDpsStatus.c_str());
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

	if (NativeStartsWith(message, "SHOWCASE|window|show")) {
		NativeUIShowcaseEnsureWindow(true);
		return true;
	}

	if (NativeStartsWith(message, "SHOWCASE|")) {
		return true;
	}

	if (NativeAchievementParseTransport(message)) {
		return true;
	}

	if (NativeMulticlassParseTransport(message)) {
		return true;
	}

	if (NativeFactionParseTransport(message)) {
		return true;
	}

	if (NativeDpsParseTransport(message)) {
		return true;
	}

	if (NativeStartsWith(message, "HPFIX|self|")) {
		return NativeHpFixApplyPayload(std::string(message + strlen("HPFIX|self|")));
	}

	if (NativeStartsWith(message, "HPFIX|window|show")) {
		NativeHpFixEnsureWindow(true);
		if (gNativeHpFixWnd) {
			gNativeHpFixWnd->SetStatus("HPFIX window opened.");
		}
		return true;
	}

	if (NativeStartsWith(message, "HPFIX|")) {
		return true;
	}

	if (NativeItemPowerParseTransport(message)) {
		return true;
	}

	if (NativeItemRarityParseTransport(message)) {
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

	if (NativeStartsWith(message, "ADVLOOT|snapshot|begin") || NativeStartsWith(message, "ADVLOOT|clear|loot")) {
		gNativeAutoLootRows.clear();
		NativeAutoLootUpdateWindow();
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|snapshot|end")) {
		NativeAutoLootUpdateWindow();
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|window|show")) {
		NativeAutoLootEnsureWindow(true);
		if (gNativeAutoLootWnd) {
			gNativeAutoLootWnd->SetStatus("AutoLoot window reopened.");
		}
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|entry|") || NativeStartsWith(message, "ADVLOOT|loot|")) {
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
		row.auto_roll = NativeToBool(NativeGetPairValue(payload, "autoroll"));
		row.free_grab = NativeToBool(NativeGetPairValue(payload, "freegrab"));
		row.eligible = NativeToBool(NativeGetPairValue(payload, "eligible"));
		row.manage = NativeToBool(NativeGetPairValue(payload, "manage"));
		row.can_loot = NativeToBool(NativeGetPairValue(payload, "canloot"));
		row.can_vote = NativeToBool(NativeGetPairValue(payload, "canvote"));
		row.can_ask = NativeToBool(NativeGetPairValue(payload, "canask"));
		row.can_roll = NativeToBool(NativeGetPairValue(payload, "canroll"));
		row.can_freegrab = NativeToBool(NativeGetPairValue(payload, "canfreegrab"));
		row.can_give = NativeToBool(NativeGetPairValue(payload, "cangive"));
		row.can_leave = NativeToBool(NativeGetPairValue(payload, "canleave"));
		row.roll_seconds = NativeToInt(NativeGetPairValue(payload, "rollseconds"));
		row.need_count = NativeToInt(NativeGetPairValue(payload, "need"));
		row.greed_count = NativeToInt(NativeGetPairValue(payload, "greed"));
		row.no_count = NativeToInt(NativeGetPairValue(payload, "no"));
		row.waiting_count = NativeToInt(NativeGetPairValue(payload, "waiting"));
		row.owner = NativeGetPairValue(payload, "owner");
		row.assignee = NativeGetPairValue(payload, "assignee");
		row.master_name = NativeGetPairValue(payload, "mastername");
		row.lock_reason = NativeGetPairValue(payload, "lockreason");
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
		row.status_kind = NativeGetPairValue(payload, "statuskind");
		if (row.status_kind.empty()) {
			row.status_kind = row.state;
		}
		row.vote = NativeGetPairValue(payload, "vote");
		if (row.vote.empty()) {
			if (
				row.state == "need" ||
				row.state == "greed" ||
				row.state == "pass" ||
				row.state == "no" ||
				row.state == "alwaysneed" ||
				row.state == "alwaysgreed"
			) {
				row.vote = row.state;
			}
			else {
				row.vote = "-";
			}
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

	if (NativeStartsWith(message, "ADVLOOT|status|")) {
		const std::string payload(message + strlen("ADVLOOT|status|"));
		gNativeAutoLootEnabled = NativeToBool(NativeGetPairValue(payload, "enabled"));
		gNativeAutoLootApplyFilters = NativeToBool(NativeGetPairValue(payload, "applyfilters"));
		gNativeAutoLootGrouped = NativeToBool(NativeGetPairValue(payload, "grouped"));
		gNativeAutoLootLeader = NativeToBool(NativeGetPairValue(payload, "leader"));
		gNativeAutoLootAlwaysNeedCount = NativeToInt(NativeGetPairValue(payload, "alwaysneed"));
		gNativeAutoLootAlwaysGreedCount = NativeToInt(NativeGetPairValue(payload, "alwaysgreed"));
		gNativeAutoLootNeverCount = NativeToInt(NativeGetPairValue(payload, "never"));
		gNativeAutoLootAutoRollCount = NativeToInt(NativeGetPairValue(payload, "autoroll"));
		gNativeAutoLootMasterCandidate = NativeToBool(NativeGetPairValue(payload, "mastercandidate"));
		gNativeAutoLootAutoSplit = NativeToBool(NativeGetPairValue(payload, "autosplit"));
		gNativeAutoLootAutoLootAll = NativeToBool(NativeGetPairValue(payload, "autolootall"));
		gNativeAutoLootAutoShow = NativeToBool(NativeGetPairValue(payload, "autoshow"));
		gNativeAutoLootShowNewOnly = NativeToBool(NativeGetPairValue(payload, "shownewonly"));
		gNativeAutoLootConfirmRemove = NativeToBool(NativeGetPairValue(payload, "confirmremove"));
		gNativeAutoLootAutoRemoveLore = NativeToBool(NativeGetPairValue(payload, "autoremovelore"));
		gNativeAutoLootDebug = NativeToBool(NativeGetPairValue(payload, "debug"));
		gNativeAutoLootLog = NativeToBool(NativeGetPairValue(payload, "log"));
		gNativeAutoLootMasterCharacterId = NativeToInt(NativeGetPairValue(payload, "master"));
		gNativeAutoLootMasterName = NativeGetPairValue(payload, "mastername");
		NativeAutoLootUpdateWindow();
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|manage|begin|")) {
		const std::string payload(message + strlen("ADVLOOT|manage|begin|"));
		gNativeAutoLootManagePlayers.clear();
		gNativeAutoLootManageEntryId = NativeToInt(NativeGetPairValue(payload, "id"));
		gNativeAutoLootManageItemName = NativeGetPairValue(payload, "name");
		gNativeAutoLootManageSource = NativeGetPairValue(payload, "source");
		gNativeAutoLootManageState = NativeGetPairValue(payload, "state");
		gNativeAutoLootManageMasterId = NativeToInt(NativeGetPairValue(payload, "master"));
		gNativeAutoLootManageMasterName = NativeGetPairValue(payload, "mastername");
		gNativeAutoLootManageCanManage = NativeToBool(NativeGetPairValue(payload, "manage"));
		gNativeAutoLootManageFreeGrab = NativeToBool(NativeGetPairValue(payload, "freegrab"));
		gNativeAutoLootManageAutoRoll = NativeToBool(NativeGetPairValue(payload, "autoroll"));
		gNativeAutoLootManageRollSeconds = NativeToInt(NativeGetPairValue(payload, "rollseconds"));
		if (gNativeAutoLootManageWnd) {
			gNativeAutoLootManageWnd->RefreshRows();
			gNativeAutoLootManageWnd->SetStatus("Loading Manage Loot...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|manage|player|")) {
		const std::string payload(message + strlen("ADVLOOT|manage|player|"));
		NativeAutoLootManagePlayer player;
		player.entry_id = NativeToInt(NativeGetPairValue(payload, "id"));
		player.character_id = NativeToInt(NativeGetPairValue(payload, "char_id"));
		player.name = NativeGetPairValue(payload, "name");
		player.vote = NativeGetPairValue(payload, "vote");
		player.master = NativeToBool(NativeGetPairValue(payload, "master"));
		player.eligible = NativeToBool(NativeGetPairValue(payload, "eligible"));
		if (player.character_id > 0 && !player.name.empty()) {
			gNativeAutoLootManagePlayers.push_back(player);
		}
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|manage|end|")) {
		if (gNativeAutoLootManageWnd) {
			gNativeAutoLootManageWnd->RefreshRows();
			gNativeAutoLootManageWnd->SetStatus("Manage Loot refreshed.");
		}
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|filters|begin")) {
		gNativeAutoLootRuleRows.clear();
		if (gNativeAutoLootRulesWnd) {
			gNativeAutoLootRulesWnd->RefreshRows();
			gNativeAutoLootRulesWnd->SetStatus("Loading filters...");
		}
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|filter|")) {
		const std::string payload(message + strlen("ADVLOOT|filter|"));
		NativeAutoLootRuleRow row;
		row.rule = NativeGetPairValue(payload, "decision");
		if (row.rule.empty()) {
			row.rule = NativeGetPairValue(payload, "mode");
		}
		row.auto_roll = NativeToBool(NativeGetPairValue(payload, "autoroll"));
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

	if (NativeStartsWith(message, "ADVLOOT|filters|end")) {
		if (gNativeAutoLootRulesWnd) {
			gNativeAutoLootRulesWnd->RefreshRows();
			gNativeAutoLootRulesWnd->SetStatus("Filters refreshed.");
		}
		return true;
	}

	if (NativeStartsWith(message, "ADVLOOT|filters|")) {
		return true;
	}

	if (strstr(message, "You say, '#advloot") || strstr(message, "You say, '#livespell") || strstr(message, "You say, '#itemforge") || strstr(message, "You say, '#ach") || strstr(message, "You say, '#rep") || strstr(message, "You say, '#ts") || strstr(message, "You say, '#tradeskill") || strstr(message, "You say, '#mc") || strstr(message, "You say, '#multiclass") || strstr(message, "You say, '#nativeui") || strstr(message, "You say, '#showcase") || strstr(message, "You say, '#hpfix")) {
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

		// The transport parsers dereference the gNative*Wnd globals. When this DLL
		// runs alongside MacroQuest the UI-reset hook is skipped (see
		// NativeAutoLootInstallUiResetHook), so a /loadskin can tear the windows
		// down without our globals being cleared. Guard the parse with SEH like the
		// pulse hook does, and fall through to the trampoline so chat still shows.
		bool transport_handled = false;
		__try {
			const bool native_interface_handled = nativeinterface::HandleChatMessage(szMsg);
			transport_handled = NativeAutoLootParseTransport(szMsg) || native_interface_handled;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			NativeAutoLootTrace("chat transport hook faulted; passing message through to client");
			transport_handled = false;
		}

		if (transport_handled) {
			return;
		}

		char stamped[4096];
		if (NativeChatTimestampFormatLine(szMsg, stamped, sizeof(stamped))) {
			Trampoline(stamped, dwColor, EqLog, dopercentsubst);
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

static void NativeAchievementResetState()
{
	gNativeAchievementCategories.clear();
	gNativeAchievementRows.clear();
	gNativeAchievementObjectives.clear();
	gNativeAchievementRewards.clear();
	gNativeAchievementSelectedCategory = 0;
	gNativeAchievementSelectedAchievement = 0;
	gNativeAchievementCompleted = 0;
	gNativeAchievementTotal = 0;
	gNativeAchievementPoints = 0;
	gNativeAchievementCategoryCount = 0;
	gNativeAchievementLoading = false;
	gNativeAchievementCategoriesDirty = true;
	gNativeAchievementRowsDirty = true;
	gNativeAchievementObjectivesDirty = true;
	gNativeAchievementRewardsDirty = true;
	gNativeAchievementDetailTitle = "Select an achievement";
	gNativeAchievementDetailDescription.clear();
}

static void NativeFactionResetState()
{
	gNativeFactionRows.clear();
	gNativeFactionStatus = "Use /rep to refresh. Target an NPC to pin its primary faction.";
	gNativeFactionTargetId = 0;
	gNativeFactionMode.clear();
	gNativeFactionSearch.clear();
	gNativeFactionLoading = false;
	gNativeFactionRowsDirty = true;
}

static void NativeDpsResetState()
{
	gNativeDpsRows.clear();
	gNativeDpsStatus = "Waiting for DPS data.";
	gNativeDpsTarget.clear();
	gNativeDpsEncounterId = 0;
	gNativeDpsElapsedMs = 0;
	gNativeDpsDamage = 0;
	gNativeDpsHealing = 0;
	gNativeDpsIncoming = 0;
	gNativeDpsLoading = false;
	gNativeDpsRowsDirty = true;
}

static void NativeAutoFollowResetState()
{
	gNativeAutoFollowEnabled = false;
	gNativeAutoFollowTargetId = 0;
	gNativeAutoFollowLastDistance = 0.0f;
	gNativeAutoFollowStuckPulses = 0;
}

static bool NativeAutoLootHasRuntimeWindows()
{
	return
		gNativeAutoLootWnd ||
		gNativeAutoLootRulesWnd ||
		gNativeAutoLootSettingsWnd ||
		gNativeAutoLootManageWnd ||
		gNativeSpellForgeWnd ||
		gNativeItemForgeWnd ||
		gNativeAchievementWnd ||
		gNativeFactionWnd ||
		gNativeDpsWnd ||
		gNativeTradeskillsWnd ||
		gNativeUIShowcaseWnd ||
		gNativeHpFixWnd ||
		gNativeMulticlassWnd ||
		gNativeMulticlassPetWnd ||
		gNativeMulticlassMelodyWnd ||
		gNativeMulticlassDisciplineWnd;
}

static void NativeAutoLootDestroyRuntimeWindows()
{
#define NATIVE_AUTOLOOT_DESTROY_WINDOW(ptr, label) \
	do { \
		if (ptr) { \
			auto* window = ptr; \
			ptr = nullptr; \
			__try { \
				window->pXWnd()->Show(0, 1); \
				delete window; \
			} \
			__except (EXCEPTION_EXECUTE_HANDLER) { \
				NativeAutoLootTrace("%s window release faulted during UI reset", label); \
			} \
		} \
	} while (0)

	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeAutoLootWnd, "AutoLoot");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeAutoLootRulesWnd, "AutoLoot rules");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeAutoLootSettingsWnd, "AutoLoot settings");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeAutoLootManageWnd, "AutoLoot manage");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeSpellForgeWnd, "Spell Forge");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeItemForgeWnd, "Item Forge");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeAchievementWnd, "Achievement");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeFactionWnd, "Faction reputation");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeDpsWnd, "DPS parser");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeTradeskillsWnd, "Tradeskills helper");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeUIShowcaseWnd, "Native UI showcase");
	NATIVE_AUTOLOOT_DESTROY_WINDOW(gNativeHpFixWnd, "HP Fix");
	NativeMulticlassDestroyRuntimeWindows();
	gNativeAutoLootDragItemAnimation = nullptr;
	gNativeAutoLootCheckNormalAnimation = nullptr;
	gNativeAutoLootCheckPressedAnimation = nullptr;
	gNativeAutoLootCloseAnimation = nullptr;

#undef NATIVE_AUTOLOOT_DESTROY_WINDOW
}

static void NativeAutoLootResetSessionRequests()
{
	gNativeAutoLootInGamePulses = 0;
	gNativeAutoLootRequestedInitialStatus = false;
	gNativeAutoLootWasInGame = false;
	gNativeAutoLootWindowConstructionFaulted = false;
	gNativeAutoLootRows.clear();
	gNativeAutoLootRuleRows.clear();
	gNativeAutoLootManagePlayers.clear();
	gNativeAutoLootManageEntryId = 0;
	gNativeAutoLootManageMasterId = 0;
	gNativeAutoLootManageRollSeconds = 0;
	gNativeAutoLootManageCanManage = false;
	gNativeAutoLootManageFreeGrab = false;
	gNativeAutoLootManageAutoRoll = false;
	gNativeAutoLootManageItemName.clear();
	gNativeAutoLootManageSource.clear();
	gNativeAutoLootManageState.clear();
	gNativeAutoLootManageMasterName.clear();
	gNativeAutoLootEnabled = false;
	gNativeAutoLootApplyFilters = true;
	gNativeAutoLootGrouped = false;
	gNativeAutoLootLeader = false;
	gNativeAutoLootMasterCandidate = true;
	gNativeAutoLootAutoSplit = true;
	gNativeAutoLootAutoLootAll = false;
	gNativeAutoLootAutoShow = true;
	gNativeAutoLootShowNewOnly = true;
	gNativeAutoLootConfirmRemove = true;
	gNativeAutoLootAutoRemoveLore = true;
	gNativeAutoLootDebug = false;
	gNativeAutoLootLog = false;
	gNativeAutoLootMasterCharacterId = 0;
	gNativeAutoLootMasterName.clear();
	gNativeAutoLootAlwaysNeedCount = 0;
	gNativeAutoLootAlwaysGreedCount = 0;
	gNativeAutoLootNeverCount = 0;
	gNativeAutoLootAutoRollCount = 0;
	gNativeLiveSpellSentReady = false;
	gNativeHpFixSentReady = false;
	gNativeHpFixReadyRetryPulses = 0;
	gNativeHpFixRefreshPulses = 0;
	gNativeHpFixState = NativeHpFixState();
	gNativeItemPowerById.clear();
	gNativeItemRarityById.clear();
	gNativeMulticlassSentStatus = false;
	NativeMulticlassResetSessionState(true);
	NativeAchievementResetState();
	NativeFactionResetState();
	NativeDpsResetState();
	NativeAutoFollowResetState();
}

static void NativeAutoLootResetClientUiSession(const char* reason)
{
	NativeAutoLootTrace("client UI reset: %s", reason ? reason : "unknown");
	NativeAutoLootDestroyRuntimeWindows();
	gNativeAutoLootInGamePulses = 0;
	gNativeAutoLootRequestedInitialStatus = false;
	gNativeAutoLootWasInGame = false;
	gNativeAutoLootWindowConstructionFaulted = false;
	gNativeAutoLootRows.clear();
	gNativeAutoLootRuleRows.clear();
	gNativeAutoLootManagePlayers.clear();
	gNativeAutoLootManageEntryId = 0;
	gNativeAutoLootManageMasterId = 0;
	gNativeAutoLootManageRollSeconds = 0;
	gNativeAutoLootManageCanManage = false;
	gNativeAutoLootManageFreeGrab = false;
	gNativeAutoLootManageAutoRoll = false;
	gNativeAutoLootManageItemName.clear();
	gNativeAutoLootManageSource.clear();
	gNativeAutoLootManageState.clear();
	gNativeAutoLootManageMasterName.clear();
	gNativeAutoLootEnabled = false;
	gNativeAutoLootApplyFilters = true;
	gNativeAutoLootGrouped = false;
	gNativeAutoLootLeader = false;
	gNativeAutoLootMasterCandidate = true;
	gNativeAutoLootAutoSplit = true;
	gNativeAutoLootAutoLootAll = false;
	gNativeAutoLootAutoShow = true;
	gNativeAutoLootShowNewOnly = true;
	gNativeAutoLootConfirmRemove = true;
	gNativeAutoLootAutoRemoveLore = true;
	gNativeAutoLootDebug = false;
	gNativeAutoLootLog = false;
	gNativeAutoLootMasterCharacterId = 0;
	gNativeAutoLootMasterName.clear();
	gNativeAutoLootAlwaysNeedCount = 0;
	gNativeAutoLootAlwaysGreedCount = 0;
	gNativeAutoLootNeverCount = 0;
	gNativeAutoLootAutoRollCount = 0;
	gNativeLiveSpellSentReady = false;
	gNativeHpFixSentReady = false;
	gNativeHpFixReadyRetryPulses = 0;
	gNativeHpFixRefreshPulses = 0;
	gNativeHpFixState = NativeHpFixState();
	gNativeItemPowerById.clear();
	gNativeItemRarityById.clear();
	gNativeMulticlassSentStatus = false;
	NativeMulticlassResetSessionState(false);
	NativeAchievementResetState();
	NativeFactionResetState();
	NativeDpsResetState();
	NativeAutoFollowResetState();
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
		NativeAutoLootSendCommand("/say #advloot native status");
	}

	if (!isThjClientEnabled) {
		if (!gNativeLiveSpellSentReady) {
			gNativeLiveSpellSentReady = true;
			NativeAutoLootSendCommand("/say #livespell ready");
		}

		if (!gNativeHpFixSentReady) {
			gNativeHpFixSentReady = true;
			NativeAutoLootSendCommand("/say #hpfix native ready");
		}

		if (!gNativeMulticlassSentStatus) {
			gNativeMulticlassSentStatus = true;
			NativeAutoLootSendCommand("/say #mc status");
		}
	}
}

static void NativeAutoLootPulse()
{
	const DWORD state = GetGameState();
	if (state != GAMESTATE_INGAME) {
		if (gNativeAutoLootWasInGame || NativeAutoLootHasRuntimeWindows()) {
			NativeAutoLootResetClientUiSession("left game state");
		}
		else {
			NativeAutoLootResetSessionRequests();
		}
		return;
	}

	if (!pSidlMgr || !pWndMgr) {
		NativeAutoLootResetSessionRequests();
		return;
	}

	if (!pEverQuest || !pLocalPlayer || !GetCharInfo2()) {
		NativeAutoLootResetSessionRequests();
		return;
	}

	gNativeAutoLootWasInGame = true;

	if (gNativeAutoLootInGamePulses < 120) {
		++gNativeAutoLootInGamePulses;
		return;
	}

	NativeAutoLootInstallChatHook();

	if (gNativeAutoLootWnd) {
		gNativeAutoLootWnd->Layout();
		gNativeAutoLootWnd->DiagnosticPulse();
	}

	if (gNativeAutoLootRulesWnd) {
		gNativeAutoLootRulesWnd->Layout();
	}

	if (gNativeAutoLootSettingsWnd) {
		gNativeAutoLootSettingsWnd->Layout();
	}

	if (gNativeAutoLootManageWnd) {
		gNativeAutoLootManageWnd->PulseTick();
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

	if (gNativeFactionWnd) {
		gNativeFactionWnd->Layout();
	}

	if (gNativeDpsWnd) {
		gNativeDpsWnd->Layout();
	}

	if (gNativeTradeskillsWnd) {
		gNativeTradeskillsWnd->Layout();
	}

	if (gNativeUIShowcaseWnd) {
		gNativeUIShowcaseWnd->Layout();
	}

	if (gNativeHpFixWnd) {
		gNativeHpFixWnd->Refresh();
	}

	if (!isThjClientEnabled) {
		NativeHpFixPulseSync();
		NativeHpFixMaintainNormalUi();
	}

	if (gNativeMulticlassWnd) {
		gNativeMulticlassWnd->Layout();
	}

	if (gNativeMulticlassPetWnd) {
		gNativeMulticlassPetWnd->Layout();
	}

	if (gNativeMulticlassMelodyWnd) {
		gNativeMulticlassMelodyWnd->Layout();
	}

	if (gNativeMulticlassDisciplineWnd) {
		gNativeMulticlassDisciplineWnd->Layout();
	}

	if (!isThjClientEnabled) {
		NativeMulticlassMaintainPresentationUI();
		NativeMulticlassMaintainSpellLevelPatches();
		NativeAutoFollowPulse();
	}

	if (!gNativeAutoLootRequestedInitialStatus) {
		gNativeAutoLootRequestedInitialStatus = true;
		NativeAutoLootSendCommand("/say #advloot native status");
	}

	if (!isThjClientEnabled) {
		if (!gNativeLiveSpellSentReady) {
			gNativeLiveSpellSentReady = true;
			NativeAutoLootSendCommand("/say #livespell ready");
		}

		if (!gNativeHpFixSentReady) {
			gNativeHpFixSentReady = true;
			NativeAutoLootSendCommand("/say #hpfix native ready");
		}

		if (!gNativeMulticlassSentStatus) {
			gNativeMulticlassSentStatus = true;
			NativeAutoLootSendCommand("/say #mc status");
		}
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

class NativeAutoLootUiResetHook
{
public:
	VOID CleanUI_Trampoline(VOID);
	VOID CleanUI_Detour(VOID)
	{
		NativeAutoLootResetClientUiSession("clean game UI");
		CleanUI_Trampoline();
	}

	VOID ReloadUI_Trampoline(BOOL use_ini);
	VOID ReloadUI_Detour(BOOL use_ini)
	{
		NativeAutoLootResetClientUiSession("reload UI");
		ReloadUI_Trampoline(use_ini);
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID NativeAutoLootUiResetHook::CleanUI_Trampoline(VOID));
DETOUR_TRAMPOLINE_EMPTY(VOID NativeAutoLootUiResetHook::ReloadUI_Trampoline(BOOL));

static void NativeAutoLootInstallUiResetHook()
{
	if (gNativeAutoLootUiResetHookInstalled) {
		return;
	}

	if (isMQInjectsEnabled) {
		NativeAutoLootTrace("native UI reset hook skipped because MQ display hook is enabled");
		return;
	}

	NativeAutoLootTrace("installing native UI reset hook");
	EzDetour(CDisplay__CleanGameUI, &NativeAutoLootUiResetHook::CleanUI_Detour, &NativeAutoLootUiResetHook::CleanUI_Trampoline);
	EzDetour(CDisplay__ReloadUI, &NativeAutoLootUiResetHook::ReloadUI_Detour, &NativeAutoLootUiResetHook::ReloadUI_Trampoline);
	gNativeAutoLootUiResetHookInstalled = true;
}

static void InitAutoLootNative()
{
	if (gNativeAutoLootHooksInstalled) {
		return;
	}

	gNativeAutoLootHooksInstalled = true;
	NativeAutoLootInstallChatHook();
	NativeAutoLootInstallCommandHook();
	if (!isThjClientEnabled) {
		NativeMulticlassInstallContextMenuHook();
	}
	NativeAutoLootInstallUiResetHook();

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

	if (gNativeAutoLootSettingsWnd) {
		delete gNativeAutoLootSettingsWnd;
		gNativeAutoLootSettingsWnd = nullptr;
	}

	if (gNativeAutoLootManageWnd) {
		delete gNativeAutoLootManageWnd;
		gNativeAutoLootManageWnd = nullptr;
	}

	if (gNativeAutoLootMenuWnd) {
		delete gNativeAutoLootMenuWnd;
		gNativeAutoLootMenuWnd = nullptr;
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

	if (gNativeFactionWnd) {
		delete gNativeFactionWnd;
		gNativeFactionWnd = nullptr;
	}

	if (gNativeDpsWnd) {
		delete gNativeDpsWnd;
		gNativeDpsWnd = nullptr;
	}

	if (gNativeTradeskillsWnd) {
		delete gNativeTradeskillsWnd;
		gNativeTradeskillsWnd = nullptr;
	}

	if (gNativeUIShowcaseWnd) {
		delete gNativeUIShowcaseWnd;
		gNativeUIShowcaseWnd = nullptr;
	}

	if (gNativeHpFixWnd) {
		delete gNativeHpFixWnd;
		gNativeHpFixWnd = nullptr;
	}

	if (gNativeMulticlassWnd) {
		delete gNativeMulticlassWnd;
		gNativeMulticlassWnd = nullptr;
	}

	if (gNativeMulticlassPetWnd) {
		delete gNativeMulticlassPetWnd;
		gNativeMulticlassPetWnd = nullptr;
	}

	if (gNativeMulticlassMelodyWnd) {
		delete gNativeMulticlassMelodyWnd;
		gNativeMulticlassMelodyWnd = nullptr;
	}

	if (gNativeMulticlassDisciplineWnd) {
		delete gNativeMulticlassDisciplineWnd;
		gNativeMulticlassDisciplineWnd = nullptr;
	}

	gNativeAutoLootDragItemAnimation = nullptr;
	gNativeAutoLootCheckNormalAnimation = nullptr;
	gNativeAutoLootCheckPressedAnimation = nullptr;
	gNativeAutoLootCloseAnimation = nullptr;

	if (gNativeAutoLootChatHookInstalled) {
		RemoveDetour(CEverQuest__dsp_chat);
		gNativeAutoLootChatHookInstalled = false;
	}

	if (gNativeAutoLootCommandHookInstalled) {
		RemoveDetour(CEverQuest__InterpretCmd);
		gNativeAutoLootCommandHookInstalled = false;
	}

	if (gNativeMulticlassContextMenuHookInstalled) {
		RemoveDetour(NativeMulticlassContextMenuAddMenuItemAddress());
		gNativeMulticlassContextMenuHookInstalled = false;
	}

	if (gNativeAutoLootUiResetHookInstalled) {
		RemoveDetour(CDisplay__CleanGameUI);
		RemoveDetour(CDisplay__ReloadUI);
		gNativeAutoLootUiResetHookInstalled = false;
	}

	if (gNativeAutoLootPulseHookInstalled) {
		RemoveDetour((DWORD)ProcessGameEvents);
		gNativeAutoLootPulseHookInstalled = false;
	}

	gNativeAutoLootHooksInstalled = false;
	gNativeAutoLootRequestedInitialStatus = false;
	gNativeLiveSpellSentReady = false;
	gNativeHpFixSentReady = false;
	gNativeHpFixReadyRetryPulses = 0;
	gNativeHpFixRefreshPulses = 0;
	gNativeHpFixState = NativeHpFixState();
	gNativeItemPowerById.clear();
	gNativeItemRarityById.clear();
	gNativeMulticlassSentStatus = false;
	gNativeAutoLootPulseFaulted = false;
	gNativeAutoLootInGamePulses = 0;
	gNativeMulticlassPets.clear();
	gNativeMulticlassMelodySlots.clear();
	gNativeMulticlassMelodySongs.clear();
	gNativeMulticlassSpellLevelsById.clear();
	gNativeMulticlassSpellLevelsByName.clear();
	gNativeMulticlassSpellLevelPatchCount = 0;
	gNativeMulticlassSpellLevelsLoading = false;
	gNativeMulticlassSpellLevelReapplyDelay = 0;
	gNativeMulticlassSpellLevelReapplyPasses = 0;
}

#endif
