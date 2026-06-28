#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <dinput.h>

#include <cstdint>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

namespace {
	constexpr std::uintptr_t kEqImageBase = 0x400000;
	constexpr std::uintptr_t kCEverQuestDspChat = 0x51F1A0;
	constexpr std::uintptr_t kChatManagerAddText = 0x650F90;
	constexpr std::uintptr_t kChatQueueCopyText = 0x809B30;
	constexpr std::uintptr_t kCItemDisplayWndUpdateStrings = 0x69AE30;
	constexpr std::uintptr_t kCLabelDraw = 0x6B4D40;
	constexpr std::uintptr_t kConvertSayLinks = 0x4ED110;
	constexpr std::uintptr_t kCStmlWndAppendSTML = 0x886720;
	constexpr std::uintptr_t kCStmlWndSetSTMLText = 0x883E10;
	constexpr std::uintptr_t kCStmlWndForceParseNow = 0x887070;
	constexpr std::uintptr_t kCTextureFontDrawLow = 0x85FD30;
	constexpr std::uintptr_t kCTextureFontDrawWrappedText = 0x889B70;
	constexpr std::uintptr_t kCTextureFontDrawWrappedTextEx = 0x889BC0;
	constexpr std::uintptr_t kCXStrCtorCString = 0x805C20;
	constexpr std::uintptr_t kCXStrDtor = 0x465AE0;

	constexpr std::size_t kCXWndColorOffset = 0x12C;
	constexpr std::size_t kCXWndWindowTextOffset = 0x1A8;
	constexpr std::size_t kItemDisplayLabelsOffset = 0x314;
	constexpr std::size_t kItemDisplayLabelCount = 12;

	using DirectInput8CreateProc = HRESULT(WINAPI *)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
	using DllCanUnloadNowProc = HRESULT(WINAPI *)();
	using DllGetClassObjectProc = HRESULT(WINAPI *)(REFCLSID, REFIID, LPVOID *);
	using DllRegisterServerProc = HRESULT(WINAPI *)();
	using DllUnregisterServerProc = HRESULT(WINAPI *)();
	using GetdfDIJoystickProc = LPCDIDATAFORMAT(WINAPI *)();

	HMODULE g_real_dinput8 = nullptr;
	DirectInput8CreateProc g_direct_input8_create = nullptr;
	DllCanUnloadNowProc g_dll_can_unload_now = nullptr;
	DllGetClassObjectProc g_dll_get_class_object = nullptr;
	DllRegisterServerProc g_dll_register_server = nullptr;
	DllUnregisterServerProc g_dll_unregister_server = nullptr;
	GetdfDIJoystickProc g_getdf_di_joystick = nullptr;

	CRITICAL_SECTION g_rarity_lock;
	bool g_rarity_lock_ready = false;

	struct RarityInfo {
		int tier = 0;
		const char *name = "Common";
		const char *hex = "#F0F0F0";
	};

	std::unordered_map<std::uint32_t, RarityInfo> g_rarity_by_item;
	std::unordered_map<std::string, RarityInfo> g_rarity_by_name;

	struct InlineHook {
		void *target = nullptr;
		void *gateway = nullptr;
		BYTE original[16] {};
		std::size_t stolen_length = 0;
		bool installed = false;
	};

	InlineHook g_chat_hook;
	InlineHook g_chat_manager_hook;
	InlineHook g_chat_queue_copy_hook;
	InlineHook g_item_display_hook;
	InlineHook g_label_draw_hook;
	InlineHook g_saylink_convert_hook;
	InlineHook g_stml_append_hook;
	InlineHook g_stml_set_hook;
	InlineHook g_draw_low_hook;
	InlineHook g_draw_text_hook;
	InlineHook g_draw_text_ex_hook;
	bool g_logged_label_recolor = false;
	bool g_logged_append_recolor = false;
	bool g_logged_draw_low_recolor = false;
	bool g_logged_draw_recolor = false;
	bool g_logged_draw_ex_recolor = false;
	int g_append_observe_count = 0;
	int g_dsp_observe_count = 0;
	int g_saylink_observe_count = 0;
	int g_chat_manager_observe_count = 0;
	int g_chat_queue_observe_count = 0;
	int g_draw_probe_count = 0;
	volatile LONG g_chat_draw_probe_budget = 0;
	volatile LONG g_chat_queue_copy_budget = 0;

	void Trace(const char *format, ...)
	{
		char module_path[MAX_PATH] {};
		GetModuleFileNameA(nullptr, module_path, MAX_PATH);
		char *slash = strrchr(module_path, '\\');
		if (slash) {
			*(slash + 1) = '\0';
		}
		else {
			module_path[0] = '\0';
		}

		char log_path[MAX_PATH] {};
		sprintf_s(log_path, "%sitem_rarity_native.log", module_path);

		FILE *file = nullptr;
		if (fopen_s(&file, log_path, "a") || !file) {
			return;
		}

		SYSTEMTIME now {};
		GetLocalTime(&now);
		fprintf(
			file,
			"[%04u-%02u-%02u %02u:%02u:%02u] ",
			now.wYear,
			now.wMonth,
			now.wDay,
			now.wHour,
			now.wMinute,
			now.wSecond
		);

		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		va_end(args);

		fprintf(file, "\n");
		fclose(file);
	}

	std::uintptr_t Rebase(std::uintptr_t preferred_address)
	{
		const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
		return base + (preferred_address - kEqImageBase);
	}

	bool InstallHook(InlineHook &hook, void *target, void *detour, std::size_t stolen_length)
	{
		if (hook.installed) {
			return true;
		}

		if (stolen_length < 5 || stolen_length > sizeof(hook.original)) {
			return false;
		}

		hook.target = target;
		hook.stolen_length = stolen_length;
		memcpy(hook.original, target, hook.stolen_length);

		auto *gateway = static_cast<BYTE *>(VirtualAlloc(nullptr, hook.stolen_length + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (!gateway) {
			return false;
		}

		memcpy(gateway, hook.original, hook.stolen_length);
		gateway[hook.stolen_length] = 0xE9;
		*reinterpret_cast<std::int32_t *>(gateway + hook.stolen_length + 1) = static_cast<std::int32_t>(
			(reinterpret_cast<std::uintptr_t>(target) + hook.stolen_length) - (reinterpret_cast<std::uintptr_t>(gateway) + hook.stolen_length + 5)
		);

		DWORD old_protect = 0;
		if (!VirtualProtect(target, hook.stolen_length, PAGE_EXECUTE_READWRITE, &old_protect)) {
			VirtualFree(gateway, 0, MEM_RELEASE);
			return false;
		}

		auto *patch = static_cast<BYTE *>(target);
		memset(patch, 0x90, hook.stolen_length);
		patch[0] = 0xE9;
		*reinterpret_cast<std::int32_t *>(patch + 1) = static_cast<std::int32_t>(
			reinterpret_cast<std::uintptr_t>(detour) - reinterpret_cast<std::uintptr_t>(target) - 5
		);

		VirtualProtect(target, hook.stolen_length, old_protect, &old_protect);
		FlushInstructionCache(GetCurrentProcess(), target, hook.stolen_length);

		hook.gateway = gateway;
		hook.installed = true;
		return true;
	}

	void RemoveHook(InlineHook &hook)
	{
		if (!hook.installed || !hook.target) {
			return;
		}

		DWORD old_protect = 0;
		if (VirtualProtect(hook.target, hook.stolen_length, PAGE_EXECUTE_READWRITE, &old_protect)) {
			memcpy(hook.target, hook.original, hook.stolen_length);
			VirtualProtect(hook.target, hook.stolen_length, old_protect, &old_protect);
			FlushInstructionCache(GetCurrentProcess(), hook.target, hook.stolen_length);
		}

		if (hook.gateway) {
			VirtualFree(hook.gateway, 0, MEM_RELEASE);
		}

		hook = InlineHook {};
	}

	const RarityInfo &RarityForTier(int tier)
	{
		static const RarityInfo kRarities[] = {
			{0, "Common", "#F0F0F0"},
			{1, "Uncommon", "#66FF66"},
			{2, "Rare", "#00FFFF"},
			{3, "Legendary", "#FFD15C"},
			{4, "Unique", "#C080FF"}
		};

		if (tier < 0 || tier > 4) {
			return kRarities[0];
		}

		return kRarities[tier];
	}

	bool StartsWith(const char *value, const char *prefix)
	{
		return value && prefix && strncmp(value, prefix, strlen(prefix)) == 0;
	}

	bool ReadPairUInt(const char *message, const char *key, std::uint32_t &value)
	{
		const char *found = strstr(message, key);
		if (!found) {
			return false;
		}

		found += strlen(key);
		char *end = nullptr;
		const auto parsed = strtoul(found, &end, 10);
		if (end == found) {
			return false;
		}

		value = static_cast<std::uint32_t>(parsed);
		return true;
	}

	std::string ReadPairStringToEnd(const char *message, const char *key)
	{
		const char *found = strstr(message, key);
		if (!found) {
			return {};
		}

		found += strlen(key);
		return found;
	}

	bool ParseRarityTransport(const char *message)
	{
		if (!StartsWith(message, "ITEMRARITY|")) {
			return false;
		}

		std::uint32_t item_id = 0;
		if (!ReadPairUInt(message, "item_id=", item_id) || !item_id) {
			return true;
		}

		if (StartsWith(message, "ITEMRARITY|clear|")) {
			const auto item_name = ReadPairStringToEnd(message, "name=");
			if (g_rarity_lock_ready) {
				EnterCriticalSection(&g_rarity_lock);
				g_rarity_by_item.erase(item_id);
				if (!item_name.empty()) {
					g_rarity_by_name.erase(item_name);
				}
				LeaveCriticalSection(&g_rarity_lock);
			}
			Trace("cleared rarity cache for item %u name=%s", item_id, item_name.c_str());
			return true;
		}

		std::uint32_t tier = 0;
		if (!ReadPairUInt(message, "rarity=", tier) || tier > 4) {
			return true;
		}

		const auto item_name = ReadPairStringToEnd(message, "name=");
		if (g_rarity_lock_ready) {
			EnterCriticalSection(&g_rarity_lock);
			g_rarity_by_item[item_id] = RarityForTier(static_cast<int>(tier));
			if (!item_name.empty()) {
				g_rarity_by_name[item_name] = RarityForTier(static_cast<int>(tier));
			}
			LeaveCriticalSection(&g_rarity_lock);
		}

		Trace("cached rarity item=%u tier=%u name=%s", item_id, tier, item_name.c_str());
		return true;
	}

	bool ParseHexUInt(const char *text, std::size_t length, std::uint32_t &value)
	{
		if (!text || !length || length >= 16) {
			return false;
		}

		char buffer[16] {};
		memcpy(buffer, text, length);
		char *end = nullptr;
		const auto parsed = strtoul(buffer, &end, 16);
		if (end == buffer) {
			return false;
		}

		value = static_cast<std::uint32_t>(parsed);
		return true;
	}

#pragma pack(push, 1)
	struct CXStr {
		void *ptr;
	};

	struct CXStrRep {
		DWORD font;
		DWORD max_length;
		DWORD length;
		BOOL encoding;
		CRITICAL_SECTION *lock;
		char text[1];
	};

	struct ItemInfo {
		char name[0x40];
		BYTE pad_to_item_number[0xAC];
		DWORD item_number;
	};

	struct Contents {
		BYTE pad_to_item1[0x9C];
		ItemInfo *item1;
		BYTE pad_to_item2[0xA4];
		ItemInfo *item2;
	};

	struct ItemDisplayWindow {
		BYTE pad_to_display_wnd[0x220];
		void *display_wnd;
		BYTE pad_to_item_info[0x6C];
		void *item_info;
		void *window_title;
		BYTE pad_to_item[0x14];
		Contents *item;
		BYTE pad_to_item_info_labels[0x64];
		void *item_info_labels[kItemDisplayLabelCount];
	};
#pragma pack(pop)

	static_assert(sizeof(CXStr) == 0x4, "CXStr wrapper size mismatch");
	static_assert(offsetof(ItemInfo, item_number) == 0xEC, "ItemInfo item_number offset mismatch");
	static_assert(offsetof(Contents, item1) == 0x9C, "Contents item1 offset mismatch");
	static_assert(offsetof(Contents, item2) == 0x144, "Contents item2 offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, display_wnd) == 0x220, "ItemDisplayWindow display_wnd offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, item_info) == 0x290, "ItemDisplayWindow item_info offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, window_title) == 0x294, "ItemDisplayWindow window_title offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, item) == 0x2AC, "ItemDisplayWindow item offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, item_info_labels) == 0x314, "ItemDisplayWindow item info labels offset mismatch");

	ItemInfo *GetItemInfo(Contents *contents)
	{
		if (!contents) {
			return nullptr;
		}

		return contents->item1 ? contents->item1 : contents->item2;
	}

	bool LookupRarity(std::uint32_t item_id, RarityInfo &rarity)
	{
		if (!item_id || !g_rarity_lock_ready) {
			return false;
		}

		bool found = false;
		EnterCriticalSection(&g_rarity_lock);
		const auto iter = g_rarity_by_item.find(item_id);
		if (iter != g_rarity_by_item.end()) {
			rarity = iter->second;
			found = true;
		}
		LeaveCriticalSection(&g_rarity_lock);

		return found;
	}

	bool LookupRarityByName(const char *item_name, RarityInfo &rarity)
	{
		if (!item_name || !item_name[0] || !g_rarity_lock_ready) {
			return false;
		}

		bool found = false;
		EnterCriticalSection(&g_rarity_lock);
		const auto iter = g_rarity_by_name.find(item_name);
		if (iter != g_rarity_by_name.end()) {
			rarity = iter->second;
			found = true;
		}
		LeaveCriticalSection(&g_rarity_lock);

		return found;
	}

	const char *GetCXStrText(void *cxstr)
	{
		if (!cxstr) {
			return nullptr;
		}

		auto *rep = static_cast<CXStrRep *>(cxstr);
		if (!rep->text[0] || rep->encoding) {
			return nullptr;
		}

		return rep->text;
	}

	bool IsReadableMemory(const void *address, std::size_t size)
	{
		if (!address || !size) {
			return false;
		}

		MEMORY_BASIC_INFORMATION info {};
		if (!VirtualQuery(address, &info, sizeof(info))) {
			return false;
		}

		if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
			return false;
		}

		const auto begin = reinterpret_cast<std::uintptr_t>(address);
		const auto end = begin + size;
		const auto region_end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
		return end > begin && end <= region_end;
	}

	const char *GetCXStrTextSafe(const CXStr *value)
	{
		if (!IsReadableMemory(value, sizeof(CXStr))) {
			return nullptr;
		}

		if (!IsReadableMemory(value->ptr, offsetof(CXStrRep, text) + 1)) {
			return nullptr;
		}

		auto *rep = static_cast<CXStrRep *>(value->ptr);
		if (rep->encoding || rep->length > 2048 || rep->max_length > 4096) {
			return nullptr;
		}

		if (!IsReadableMemory(rep->text, rep->length + 1)) {
			return nullptr;
		}

		return rep->text[0] ? rep->text : nullptr;
	}

	const char *GetWindowText(void *window)
	{
		if (!window) {
			return nullptr;
		}

		auto *bytes = static_cast<BYTE *>(window);
		return GetCXStrText(*reinterpret_cast<void **>(bytes + kCXWndWindowTextOffset));
	}

	void SetWindowColor(void *window, DWORD argb)
	{
		if (!window) {
			return;
		}

		*reinterpret_cast<DWORD *>(static_cast<BYTE *>(window) + kCXWndColorOffset) = argb;
	}

	std::string ColorizeFirstItemName(const char *stml, const char *item_name, const RarityInfo &rarity)
	{
		if (!stml || !stml[0] || !item_name || !item_name[0]) {
			return {};
		}

		const std::string text(stml);
		const std::string name(item_name);
		const auto name_pos = text.find(name);
		if (name_pos == std::string::npos) {
			return {};
		}

		const auto tag_start = text.rfind("<c ", name_pos);
		if (tag_start != std::string::npos) {
			const auto tag_end = text.find('>', tag_start);
			const auto close_before_name = text.rfind("</c>", name_pos);
			if (tag_end != std::string::npos && tag_end < name_pos && (close_before_name == std::string::npos || close_before_name < tag_start)) {
				std::string result = text.substr(0, tag_start);
				result += "<c \"";
				result += rarity.hex;
				result += "\">";
				result += text.substr(tag_end + 1);
				return result;
			}
		}

		std::string result = text.substr(0, name_pos);
		result += "<c \"";
		result += rarity.hex;
		result += "\">";
		result += name;
		result += "</c>";
		result += text.substr(name_pos + name.length());
		return result;
	}

	bool LooksLikeItemDisplaySTML(const char *stml)
	{
		return stml && strstr(stml, "Class:") && strstr(stml, "Race:");
	}

	bool ColorizeCachedItemName(const char *stml, std::string &colorized, std::string &matched_name, RarityInfo &rarity)
	{
		if (!LooksLikeItemDisplaySTML(stml) || !g_rarity_lock_ready) {
			return false;
		}

		EnterCriticalSection(&g_rarity_lock);
		for (const auto &entry : g_rarity_by_name) {
			colorized = ColorizeFirstItemName(stml, entry.first.c_str(), entry.second);
			if (!colorized.empty()) {
				matched_name = entry.first;
				rarity = entry.second;
				LeaveCriticalSection(&g_rarity_lock);
				return true;
			}
		}
		LeaveCriticalSection(&g_rarity_lock);

		return false;
	}

	bool ColorizeCachedRenderedItemName(const char *stml, std::string &colorized, std::string &matched_name, RarityInfo &rarity)
	{
		if (!stml || !stml[0] || !g_rarity_lock_ready) {
			return false;
		}

		EnterCriticalSection(&g_rarity_lock);
		for (const auto &entry : g_rarity_by_name) {
			colorized = ColorizeFirstItemName(stml, entry.first.c_str(), entry.second);
			if (!colorized.empty()) {
				matched_name = entry.first;
				rarity = entry.second;
				LeaveCriticalSection(&g_rarity_lock);
				return true;
			}
		}
		LeaveCriticalSection(&g_rarity_lock);

		return false;
	}

	bool LookupRarityForRenderedText(const char *rendered_text, std::string &matched_name, RarityInfo &rarity)
	{
		if (!rendered_text || !rendered_text[0] || !g_rarity_lock_ready) {
			return false;
		}

		EnterCriticalSection(&g_rarity_lock);
		const auto exact = g_rarity_by_name.find(rendered_text);
		if (exact != g_rarity_by_name.end()) {
			matched_name = exact->first;
			rarity = exact->second;
			LeaveCriticalSection(&g_rarity_lock);
			return true;
		}

		for (const auto &entry : g_rarity_by_name) {
			if (!entry.first.empty() && strstr(rendered_text, entry.first.c_str())) {
				matched_name = entry.first;
				rarity = entry.second;
				LeaveCriticalSection(&g_rarity_lock);
				return true;
			}
		}
		LeaveCriticalSection(&g_rarity_lock);

		return false;
	}

	std::size_t CachedNameCount()
	{
		if (!g_rarity_lock_ready) {
			return 0;
		}

		EnterCriticalSection(&g_rarity_lock);
		const auto count = g_rarity_by_name.size();
		LeaveCriticalSection(&g_rarity_lock);
		return count;
	}

	bool TextMentionsCachedName(const char *text)
	{
		if (!text || !text[0] || !g_rarity_lock_ready) {
			return false;
		}

		bool found = false;
		EnterCriticalSection(&g_rarity_lock);
		for (const auto &entry : g_rarity_by_name) {
			if (!entry.first.empty() && strstr(text, entry.first.c_str())) {
				found = true;
				break;
			}
		}
		LeaveCriticalSection(&g_rarity_lock);

		return found;
	}

	bool ShouldObserveText(const char *text)
	{
		return text && (strchr(text, '\x12') || strstr(text, "[Legendary]") || TextMentionsCachedName(text));
	}

	bool ColorizeRawItemLinks(const char *message, std::string &colorized)
	{
		if (!message || !message[0] || !g_rarity_lock_ready) {
			return false;
		}

		constexpr char kLinkMarker = '\x12';
		constexpr std::size_t kSayLinkBodyLength = 56;

		const std::string text(message);
		std::size_t cursor = 0;
		std::ptrdiff_t offset = 0;
		bool changed = false;
		while (cursor < text.length()) {
			const auto link_start = text.find(kLinkMarker, cursor);
			if (link_start == std::string::npos) {
				break;
			}

			const auto link_end = text.find(kLinkMarker, link_start + 1);
			if (link_end == std::string::npos) {
				break;
			}

			if (link_end > link_start + 1 + kSayLinkBodyLength) {
				const auto body_start = link_start + 1;
				const auto display_start = body_start + kSayLinkBodyLength;
				std::uint32_t item_id = 0;
				RarityInfo rarity;
				if (ParseHexUInt(text.c_str() + body_start + 1, 5, item_id) && LookupRarity(item_id, rarity)) {
					const std::string display = text.substr(display_start, link_end - display_start);
					if (display.find("<c ") == std::string::npos) {
						if (colorized.empty()) {
							colorized = text;
						}

						std::string replacement;
						replacement.reserve(display.length() + 32);
						replacement += "<c \"";
						replacement += rarity.hex;
						replacement += "\">";
						replacement += display;
						replacement += "</c>";

						const auto adjusted_display_start = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(display_start) + offset);
						colorized.replace(adjusted_display_start, display.length(), replacement);
						offset += static_cast<std::ptrdiff_t>(replacement.length()) - static_cast<std::ptrdiff_t>(display.length());
						cursor = link_end + 1;
						changed = true;
						continue;
					}
				}
			}

			cursor = link_end + 1;
		}

		return changed;
	}

	DWORD RarityARGB(const RarityInfo &rarity)
	{
		if (!rarity.hex || rarity.hex[0] != '#') {
			return 0xFFFFFFFF;
		}

		return 0xFF000000 | static_cast<DWORD>(strtoul(rarity.hex + 1, nullptr, 16));
	}

	using DspChatProc = void(__thiscall *)(void *, const char *, DWORD, bool, bool);
	using ChatManagerAddTextProc = void(__thiscall *)(void *, DWORD, CXStr);
	using ChatQueueCopyTextProc = void(__cdecl *)(char *, const char *, std::size_t);
	using ItemDisplayUpdateProc = void(__thiscall *)(void *);
	using LabelDrawProc = int(__thiscall *)(void *);
	using SayLinkConvertProc = void(__cdecl *)(CXStr *, bool);
	using AppendSTMLProc = void *(__thiscall *)(void *, void *, CXStr);
	using SetSTMLTextProc = void(__thiscall *)(void *, CXStr, bool, void *);
	using ForceParseNowProc = void(__thiscall *)(void *);
	using CXStrCtorCStringProc = CXStr *(__thiscall *)(CXStr *, const char *);
	using CXStrDtorProc = void(__thiscall *)(CXStr *);
	using DrawTextLowProc = int(__cdecl *)(void *, CXStr *, void *, void *, DWORD, DWORD, DWORD);
	using DrawWrappedTextProc = int(__thiscall *)(void *, CXStr *, int, int, int, void *, DWORD, unsigned short, int);
	using DrawWrappedTextExProc = int(__thiscall *)(void *, void *, CXStr *, int, int, int, void *, DWORD, unsigned short, int);

	bool ApplyRarityToLabel(void *label, const char *source)
	{
		const char *label_text = GetWindowText(label);
		std::string item_name;
		RarityInfo rarity;
		if (!LookupRarityForRenderedText(label_text, item_name, rarity)) {
			return false;
		}

		const DWORD rarity_argb = RarityARGB(rarity);
		SetWindowColor(label, rarity_argb);

		if (!g_logged_label_recolor) {
			g_logged_label_recolor = true;
			Trace(
				"label recolored source=%s matched=%s rendered=%s tier=%d argb=0x%08X",
				source ? source : "unknown",
				item_name.c_str(),
				label_text ? label_text : "",
				rarity.tier,
				rarity_argb
			);
		}

		return true;
	}

	int ApplyRarityToItemDisplayLabels(void *self)
	{
		if (!self) {
			return 0;
		}

		int recolored = 0;
		auto **labels = reinterpret_cast<void **>(static_cast<BYTE *>(self) + kItemDisplayLabelsOffset);
		for (std::size_t index = 0; index < kItemDisplayLabelCount; ++index) {
			if (ApplyRarityToLabel(labels[index], "item_display")) {
				++recolored;
			}
		}

		return recolored;
	}

	bool AppendSTML(void *stml_wnd, const char *text)
	{
		if (!stml_wnd || !text || !text[0]) {
			return false;
		}

		CXStr value {};
		const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
		const auto append_stml = reinterpret_cast<AppendSTMLProc>(Rebase(kCStmlWndAppendSTML));
		const auto force_parse = reinterpret_cast<ForceParseNowProc>(Rebase(kCStmlWndForceParseNow));

		ctor(&value, text);
		BYTE size_result[8] {};
		append_stml(stml_wnd, size_result, value);
		force_parse(stml_wnd);
		return true;
	}

	void *__fastcall AppendSTMLDetour(void *self, void *, void *size_result, CXStr value)
	{
		const char *text = GetCXStrText(value.ptr);
		if (g_append_observe_count < 12 && (TextMentionsCachedName(text) || (text && strstr(text, "[Legendary]")))) {
			++g_append_observe_count;
			Trace("append observed text=%s", text ? text : "<null>");
		}

		std::string colorized;
		std::string item_name;
		RarityInfo rarity;
		if (ColorizeCachedRenderedItemName(text, colorized, item_name, rarity)) {
			CXStr replacement {};
			const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
			const auto dtor = reinterpret_cast<CXStrDtorProc>(Rebase(kCXStrDtor));

			ctor(&replacement, colorized.c_str());
			void *result = reinterpret_cast<AppendSTMLProc>(g_stml_append_hook.gateway)(self, size_result, replacement);
			dtor(&replacement);

			if (!g_logged_append_recolor) {
				g_logged_append_recolor = true;
				Trace("stml append recolored item name=%s tier=%d", item_name.c_str(), rarity.tier);
			}

			return result;
		}

		return reinterpret_cast<AppendSTMLProc>(g_stml_append_hook.gateway)(self, size_result, value);
	}

	bool SetSTMLText(void *stml_wnd, const char *text)
	{
		if (!stml_wnd || !text || !text[0]) {
			return false;
		}

		CXStr value {};
		const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
		const auto set_stml = reinterpret_cast<SetSTMLTextProc>(
			g_stml_set_hook.installed && g_stml_set_hook.gateway ? g_stml_set_hook.gateway : reinterpret_cast<void *>(Rebase(kCStmlWndSetSTMLText))
		);
		const auto force_parse = reinterpret_cast<ForceParseNowProc>(Rebase(kCStmlWndForceParseNow));

		ctor(&value, text);
		set_stml(stml_wnd, value, false, nullptr);
		force_parse(stml_wnd);
		return true;
	}

	void __fastcall SetSTMLTextDetour(void *self, void *, CXStr value, bool add_to_history, void *link_info)
	{
		const char *text = GetCXStrText(value.ptr);
		std::string colorized;
		std::string item_name;
		RarityInfo rarity;
		if (ColorizeCachedItemName(text, colorized, item_name, rarity)) {
			CXStr replacement {};
			const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
			const auto dtor = reinterpret_cast<CXStrDtorProc>(Rebase(kCXStrDtor));

			ctor(&replacement, colorized.c_str());
			reinterpret_cast<SetSTMLTextProc>(g_stml_set_hook.gateway)(self, replacement, add_to_history, link_info);
			dtor(&value);
			Trace("stml set recolored item name=%s tier=%d", item_name.c_str(), rarity.tier);
			return;
		}

		if (LooksLikeItemDisplaySTML(text)) {
			Trace("stml item display observed without rarity match cached_names=%u", static_cast<unsigned>(CachedNameCount()));
		}

		reinterpret_cast<SetSTMLTextProc>(g_stml_set_hook.gateway)(self, value, add_to_history, link_info);
	}

	void __fastcall DspChatDetour(void *self, void *, const char *message, DWORD color, bool eq_log, bool do_percent_subst)
	{
		if (ParseRarityTransport(message)) {
			return;
		}

		if (g_dsp_observe_count < 12 && (TextMentionsCachedName(message) || (message && strstr(message, "[Legendary]")))) {
			++g_dsp_observe_count;
			Trace("dsp observed color=%lu message=%s", static_cast<unsigned long>(color), message ? message : "<null>");
		}
		if (message && strchr(message, '\x12') && (TextMentionsCachedName(message) || strstr(message, "[Legendary]"))) {
			InterlockedExchange(&g_chat_draw_probe_budget, 160);
			InterlockedExchange(&g_chat_queue_copy_budget, 8);
		}

		reinterpret_cast<DspChatProc>(g_chat_hook.gateway)(self, message, color, eq_log, do_percent_subst);
	}

	void __cdecl SayLinkConvertDetour(CXStr *text, bool enable_links)
	{
		const char *before = GetCXStrTextSafe(text);
		if (!before && text) {
			before = GetCXStrText(text->ptr);
		}
		const bool observe = g_saylink_observe_count < 16 && ShouldObserveText(before);
		if (observe) {
			++g_saylink_observe_count;
			Trace("saylink convert before enable=%d text=%s", enable_links ? 1 : 0, before ? before : "<null>");
		}

		reinterpret_cast<SayLinkConvertProc>(g_saylink_convert_hook.gateway)(text, enable_links);

		const char *converted = GetCXStrTextSafe(text);
		if (!converted && text) {
			converted = GetCXStrText(text->ptr);
		}
		if (observe) {
			Trace("saylink convert after text=%s", converted ? converted : "<null>");
		}

		std::string colorized;
		std::string item_name;
		RarityInfo rarity;
		if (!ColorizeCachedRenderedItemName(converted, colorized, item_name, rarity)) {
			return;
		}

		const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
		const auto dtor = reinterpret_cast<CXStrDtorProc>(Rebase(kCXStrDtor));
		dtor(text);
		ctor(text, colorized.c_str());

		Trace("saylink conversion recolored item name=%s tier=%d text=%s", item_name.c_str(), rarity.tier, colorized.c_str());
	}

	void __fastcall ChatManagerAddTextDetour(void *self, void *, DWORD color, CXStr value)
	{
		const char *text = GetCXStrTextSafe(&value);
		if (!text) {
			text = GetCXStrText(value.ptr);
		}

		if (g_chat_manager_observe_count < 16 && ShouldObserveText(text)) {
			++g_chat_manager_observe_count;
			Trace("chat manager observed color=%lu text=%s", static_cast<unsigned long>(color), text ? text : "<null>");
		}

		std::string colorized;
		std::string item_name;
		RarityInfo rarity;
		if (ColorizeCachedRenderedItemName(text, colorized, item_name, rarity)) {
			CXStr replacement {};
			const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
			ctor(&replacement, colorized.c_str());

			Trace("chat manager recolored item name=%s tier=%d text=%s", item_name.c_str(), rarity.tier, colorized.c_str());
			reinterpret_cast<ChatManagerAddTextProc>(g_chat_manager_hook.gateway)(self, color, replacement);
			return;
		}

		reinterpret_cast<ChatManagerAddTextProc>(g_chat_manager_hook.gateway)(self, color, value);
	}

	void __cdecl ChatQueueCopyTextDetour(char *dest, const char *src, std::size_t count)
	{
		if (g_chat_queue_copy_budget > 0 && count == 0x800 && src && src[0]) {
			const LONG remaining = InterlockedDecrement(&g_chat_queue_copy_budget);
			const bool observe = remaining >= 0 && ShouldObserveText(src);
			if (observe && g_chat_queue_observe_count < 24) {
				++g_chat_queue_observe_count;
				Trace("chat queue copy observed text=%s", src);
			}

			if (observe && !strchr(src, '\x12')) {
				std::string colorized;
				std::string item_name;
				RarityInfo rarity;
				if (ColorizeCachedRenderedItemName(src, colorized, item_name, rarity)) {
					Trace("chat queue copy recolored item name=%s tier=%d text=%s", item_name.c_str(), rarity.tier, colorized.c_str());
					reinterpret_cast<ChatQueueCopyTextProc>(g_chat_queue_copy_hook.gateway)(dest, colorized.c_str(), count);
					return;
				}
			}
		}

		reinterpret_cast<ChatQueueCopyTextProc>(g_chat_queue_copy_hook.gateway)(dest, src, count);
	}

	int __fastcall DrawWrappedTextDetour(void *self, void *, CXStr *text, int x, int y, int width, void *clip_rect, DWORD argb, unsigned short flags, int unknown)
	{
		const char *rendered_text = text ? GetCXStrText(text->ptr) : nullptr;
		std::string item_name;
		RarityInfo rarity;
		if (LookupRarityForRenderedText(rendered_text, item_name, rarity)) {
			const DWORD rarity_argb = RarityARGB(rarity);
			if (!g_logged_draw_recolor) {
				g_logged_draw_recolor = true;
				Trace("draw text recolored matched=%s rendered=%s tier=%d argb=0x%08X", item_name.c_str(), rendered_text, rarity.tier, rarity_argb);
			}

			argb = rarity_argb;
		}

		return reinterpret_cast<DrawWrappedTextProc>(g_draw_text_hook.gateway)(
			self,
			text,
			x,
			y,
			width,
			clip_rect,
			argb,
			flags,
			unknown
		);
	}

	int __fastcall DrawWrappedTextExDetour(void *self, void *, void *maybe_text, CXStr *text, int x, int y, int width, void *clip_rect, DWORD argb, unsigned short flags, int unknown)
	{
		const char *rendered_text = GetCXStrTextSafe(text);
		if (!rendered_text) {
			rendered_text = GetCXStrTextSafe(static_cast<CXStr *>(maybe_text));
		}

		if (g_draw_probe_count < 16 && ShouldObserveText(rendered_text)) {
			++g_draw_probe_count;
			Trace(
				"draw text ex observed rendered=%s argb=0x%08X maybe_text=%p text=%p x=%d y=%d width=%d flags=0x%04X unknown=%d",
				rendered_text ? rendered_text : "<null>",
				argb,
				maybe_text,
				text,
				x,
				y,
				width,
				flags,
				unknown
			);
		}

		std::string item_name;
		RarityInfo rarity;
		if (LookupRarityForRenderedText(rendered_text, item_name, rarity)) {
			const DWORD rarity_argb = RarityARGB(rarity);
			if (!g_logged_draw_ex_recolor) {
				g_logged_draw_ex_recolor = true;
				Trace("draw text ex recolored matched=%s rendered=%s tier=%d argb=0x%08X original=0x%08X", item_name.c_str(), rendered_text, rarity.tier, rarity_argb, argb);
			}

			argb = rarity_argb;
		}

		return reinterpret_cast<DrawWrappedTextExProc>(g_draw_text_ex_hook.gateway)(
			self,
			maybe_text,
			text,
			x,
			y,
			width,
			clip_rect,
			argb,
			flags,
			unknown
		);
	}

	int __cdecl DrawTextLowDetour(void *font, CXStr *text, void *rect, void *clip_rect, DWORD argb, DWORD flags, DWORD unknown)
	{
		const char *arg1_text = GetCXStrTextSafe(static_cast<CXStr *>(font));
		const char *arg2_text = GetCXStrTextSafe(text);
		if (!arg2_text && text) {
			arg2_text = GetCXStrText(text->ptr);
		}

		const char *rendered_text = arg2_text ? arg2_text : arg1_text;
		if (g_chat_draw_probe_budget > 0 && (arg1_text || arg2_text)) {
			const LONG remaining = InterlockedDecrement(&g_chat_draw_probe_budget);
			if (remaining >= 0) {
				Trace(
					"draw low probe arg1=%s arg2=%s rect=%p clip=%p argb=0x%08X flags=0x%08X unknown=0x%08X",
					arg1_text ? arg1_text : "<null>",
					arg2_text ? arg2_text : "<null>",
					rect,
					clip_rect,
					argb,
					flags,
					unknown
				);
			}
		}

		std::string item_name;
		RarityInfo rarity;
		if (LookupRarityForRenderedText(rendered_text, item_name, rarity)) {
			const DWORD rarity_argb = RarityARGB(rarity);
			if (!g_logged_draw_low_recolor) {
				g_logged_draw_low_recolor = true;
				Trace("draw low recolored matched=%s rendered=%s tier=%d argb=0x%08X original=0x%08X", item_name.c_str(), rendered_text, rarity.tier, rarity_argb, argb);
			}

			argb = rarity_argb;
		}

		return reinterpret_cast<DrawTextLowProc>(g_draw_low_hook.gateway)(
			font,
			text,
			rect,
			clip_rect,
			argb,
			flags,
			unknown
		);
	}

	int __fastcall LabelDrawDetour(void *self, void *)
	{
		ApplyRarityToLabel(self, "label_draw");
		return reinterpret_cast<LabelDrawProc>(g_label_draw_hook.gateway)(self);
	}

	void __fastcall ItemDisplayUpdateDetour(void *self, void *)
	{
		reinterpret_cast<ItemDisplayUpdateProc>(g_item_display_hook.gateway)(self);

		const int labels_recolored = ApplyRarityToItemDisplayLabels(self);
		if (labels_recolored > 0) {
			Trace("item display labels recolored count=%d", labels_recolored);
		}
	}

	DWORD WINAPI InitThread(LPVOID)
	{
		Sleep(1000);

		const auto chat = reinterpret_cast<void *>(Rebase(kCEverQuestDspChat));
		const auto chat_manager = reinterpret_cast<void *>(Rebase(kChatManagerAddText));
		const auto chat_queue_copy = reinterpret_cast<void *>(Rebase(kChatQueueCopyText));
		const auto item_display = reinterpret_cast<void *>(Rebase(kCItemDisplayWndUpdateStrings));
		const auto label_draw = reinterpret_cast<void *>(Rebase(kCLabelDraw));
		const auto saylink_convert = reinterpret_cast<void *>(Rebase(kConvertSayLinks));
		const auto stml_append = reinterpret_cast<void *>(Rebase(kCStmlWndAppendSTML));
		const auto stml_set = reinterpret_cast<void *>(Rebase(kCStmlWndSetSTMLText));
		const auto draw_low = reinterpret_cast<void *>(Rebase(kCTextureFontDrawLow));
		const auto draw_text = reinterpret_cast<void *>(Rebase(kCTextureFontDrawWrappedText));
		const auto draw_text_ex = reinterpret_cast<void *>(Rebase(kCTextureFontDrawWrappedTextEx));

		const bool chat_ok = InstallHook(g_chat_hook, chat, reinterpret_cast<void *>(&DspChatDetour), 6);
		const bool chat_manager_ok = false;
		const bool chat_queue_copy_ok = false;
		const bool item_ok = InstallHook(g_item_display_hook, item_display, reinterpret_cast<void *>(&ItemDisplayUpdateDetour), 6);
		const bool label_ok = InstallHook(g_label_draw_hook, label_draw, reinterpret_cast<void *>(&LabelDrawDetour), 7);
		const bool convert_ok = InstallHook(g_saylink_convert_hook, saylink_convert, reinterpret_cast<void *>(&SayLinkConvertDetour), 6);
		const bool append_ok = InstallHook(g_stml_append_hook, stml_append, reinterpret_cast<void *>(&AppendSTMLDetour), 7);
		const bool stml_ok = InstallHook(g_stml_set_hook, stml_set, reinterpret_cast<void *>(&SetSTMLTextDetour), 6);
		const bool draw_low_ok = InstallHook(g_draw_low_hook, draw_low, reinterpret_cast<void *>(&DrawTextLowDetour), 5);
		const bool draw_ok = InstallHook(g_draw_text_hook, draw_text, reinterpret_cast<void *>(&DrawWrappedTextDetour), 7);
		const bool draw_ex_ok = InstallHook(g_draw_text_ex_hook, draw_text_ex, reinterpret_cast<void *>(&DrawWrappedTextExDetour), 7);

		Trace("item rarity native hooks installed chat=%d chat_manager=%d chat_queue_copy=%d item_display=%d label_draw=%d saylink_convert=%d stml_append=%d stml_set=%d draw_low=%d draw_text=%d draw_text_ex=%d", chat_ok ? 1 : 0, chat_manager_ok ? 1 : 0, chat_queue_copy_ok ? 1 : 0, item_ok ? 1 : 0, label_ok ? 1 : 0, convert_ok ? 1 : 0, append_ok ? 1 : 0, stml_ok ? 1 : 0, draw_low_ok ? 1 : 0, draw_ok ? 1 : 0, draw_ex_ok ? 1 : 0);
		return 0;
	}

	void LoadRealDInput()
	{
		if (g_real_dinput8) {
			return;
		}

		char path[MAX_PATH] {};
		GetSystemDirectoryA(path, MAX_PATH);
		strcat_s(path, "\\dinput8.dll");

		g_real_dinput8 = LoadLibraryA(path);
		if (!g_real_dinput8) {
			return;
		}

		g_direct_input8_create = reinterpret_cast<DirectInput8CreateProc>(GetProcAddress(g_real_dinput8, "DirectInput8Create"));
		g_dll_can_unload_now = reinterpret_cast<DllCanUnloadNowProc>(GetProcAddress(g_real_dinput8, "DllCanUnloadNow"));
		g_dll_get_class_object = reinterpret_cast<DllGetClassObjectProc>(GetProcAddress(g_real_dinput8, "DllGetClassObject"));
		g_dll_register_server = reinterpret_cast<DllRegisterServerProc>(GetProcAddress(g_real_dinput8, "DllRegisterServer"));
		g_dll_unregister_server = reinterpret_cast<DllUnregisterServerProc>(GetProcAddress(g_real_dinput8, "DllUnregisterServer"));
		g_getdf_di_joystick = reinterpret_cast<GetdfDIJoystickProc>(GetProcAddress(g_real_dinput8, "GetdfDIJoystick"));
	}
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(module);
			InitializeCriticalSection(&g_rarity_lock);
			g_rarity_lock_ready = true;
			LoadRealDInput();
			CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
			break;
		case DLL_PROCESS_DETACH:
			RemoveHook(g_draw_text_ex_hook);
			RemoveHook(g_draw_text_hook);
			RemoveHook(g_draw_low_hook);
			RemoveHook(g_stml_set_hook);
			RemoveHook(g_stml_append_hook);
			RemoveHook(g_saylink_convert_hook);
			RemoveHook(g_label_draw_hook);
			RemoveHook(g_chat_queue_copy_hook);
			RemoveHook(g_chat_manager_hook);
			RemoveHook(g_chat_hook);
			RemoveHook(g_item_display_hook);
			if (g_rarity_lock_ready) {
				DeleteCriticalSection(&g_rarity_lock);
				g_rarity_lock_ready = false;
			}
			if (g_real_dinput8) {
				FreeLibrary(g_real_dinput8);
				g_real_dinput8 = nullptr;
			}
			break;
	}

	return TRUE;
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN outer)
{
	LoadRealDInput();
	if (!g_direct_input8_create) {
		return E_FAIL;
	}

	return g_direct_input8_create(instance, version, riid, out, outer);
}

extern "C" HRESULT WINAPI DllCanUnloadNow()
{
	LoadRealDInput();
	return g_dll_can_unload_now ? g_dll_can_unload_now() : E_FAIL;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID *out)
{
	LoadRealDInput();
	return g_dll_get_class_object ? g_dll_get_class_object(clsid, riid, out) : E_FAIL;
}

extern "C" HRESULT WINAPI DllRegisterServer()
{
	LoadRealDInput();
	return g_dll_register_server ? g_dll_register_server() : E_FAIL;
}

extern "C" HRESULT WINAPI DllUnregisterServer()
{
	LoadRealDInput();
	return g_dll_unregister_server ? g_dll_unregister_server() : E_FAIL;
}

extern "C" LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
	LoadRealDInput();
	return g_getdf_di_joystick ? g_getdf_di_joystick() : nullptr;
}
