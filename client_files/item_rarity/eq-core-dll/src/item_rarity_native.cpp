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
	constexpr std::uintptr_t kCItemDisplayWndUpdateStrings = 0x69AE30;
	constexpr std::uintptr_t kCStmlWndAppendSTML = 0x886720;
	constexpr std::uintptr_t kCStmlWndSetSTMLText = 0x883E10;
	constexpr std::uintptr_t kCStmlWndForceParseNow = 0x887070;
	constexpr std::uintptr_t kCXStrCtorCString = 0x805C20;

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
	InlineHook g_item_display_hook;

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
		BYTE pad_to_display_wnd[0x21C];
		void *display_wnd;
		BYTE pad_to_item_info[0x70];
		void *item_info;
		void *window_title;
		BYTE pad_to_item[0x14];
		Contents *item;
	};
#pragma pack(pop)

	static_assert(sizeof(CXStr) == 0x4, "CXStr wrapper size mismatch");
	static_assert(offsetof(ItemInfo, item_number) == 0xEC, "ItemInfo item_number offset mismatch");
	static_assert(offsetof(Contents, item1) == 0x9C, "Contents item1 offset mismatch");
	static_assert(offsetof(Contents, item2) == 0x144, "Contents item2 offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, display_wnd) == 0x21C, "ItemDisplayWindow display_wnd offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, item_info) == 0x290, "ItemDisplayWindow item_info offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, window_title) == 0x294, "ItemDisplayWindow window_title offset mismatch");
	static_assert(offsetof(ItemDisplayWindow, item) == 0x2AC, "ItemDisplayWindow item offset mismatch");

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

	using DspChatProc = void(__thiscall *)(void *, const char *, DWORD, bool, bool);
	using ItemDisplayUpdateProc = void(__thiscall *)(void *);
	using AppendSTMLProc = void(__thiscall *)(void *, CXStr);
	using SetSTMLTextProc = void(__thiscall *)(void *, CXStr, bool, void *);
	using ForceParseNowProc = void(__thiscall *)(void *);
	using CXStrCtorCStringProc = CXStr *(__thiscall *)(CXStr *, const char *);

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
		append_stml(stml_wnd, value);
		force_parse(stml_wnd);
		return true;
	}

	bool SetSTMLText(void *stml_wnd, const char *text)
	{
		if (!stml_wnd || !text || !text[0]) {
			return false;
		}

		CXStr value {};
		const auto ctor = reinterpret_cast<CXStrCtorCStringProc>(Rebase(kCXStrCtorCString));
		const auto set_stml = reinterpret_cast<SetSTMLTextProc>(Rebase(kCStmlWndSetSTMLText));
		const auto force_parse = reinterpret_cast<ForceParseNowProc>(Rebase(kCStmlWndForceParseNow));

		ctor(&value, text);
		set_stml(stml_wnd, value, false, nullptr);
		force_parse(stml_wnd);
		return true;
	}

	void __fastcall DspChatDetour(void *self, void *, const char *message, DWORD color, bool eq_log, bool do_percent_subst)
	{
		if (ParseRarityTransport(message)) {
			return;
		}

		reinterpret_cast<DspChatProc>(g_chat_hook.gateway)(self, message, color, eq_log, do_percent_subst);
	}

	void __fastcall ItemDisplayUpdateDetour(void *self, void *)
	{
		reinterpret_cast<ItemDisplayUpdateProc>(g_item_display_hook.gateway)(self);

		auto *window = static_cast<ItemDisplayWindow *>(self);
		if (!window || !window->display_wnd) {
			return;
		}

		ItemInfo *item = GetItemInfo(window->item);
		if (!item || (!item->item_number && !item->name[0])) {
			return;
		}

		RarityInfo rarity;
		if (!LookupRarity(item->item_number, rarity) && !LookupRarityByName(item->name, rarity)) {
			return;
		}

		const char *item_name = item->name[0] ? item->name : "Unknown Item";
		const char *item_stml = GetCXStrText(window->item_info);
		const auto colorized = ColorizeFirstItemName(item_stml, item_name, rarity);
		if (!colorized.empty() && SetSTMLText(window->display_wnd, colorized.c_str())) {
			Trace(
				"item display recolored name item=%u tier=%d name=%s",
				item->item_number,
				rarity.tier,
				item_name
			);
			return;
		}

		char stml[256] {};
		sprintf_s(
			stml,
			"<BR><c \"%s\">%s item: %s</c><BR>",
			rarity.hex,
			rarity.name,
			item_name
		);

		if (AppendSTML(window->display_wnd, stml)) {
			Trace(
				"item display appended fallback item=%u tier=%d name=%s has_stml=%d",
				item->item_number,
				rarity.tier,
				item_name,
				item_stml ? 1 : 0
			);
		}
	}

	DWORD WINAPI InitThread(LPVOID)
	{
		Sleep(1000);

		const auto chat = reinterpret_cast<void *>(Rebase(kCEverQuestDspChat));
		const auto item_display = reinterpret_cast<void *>(Rebase(kCItemDisplayWndUpdateStrings));

		const bool chat_ok = InstallHook(g_chat_hook, chat, reinterpret_cast<void *>(&DspChatDetour), 6);
		const bool item_ok = InstallHook(g_item_display_hook, item_display, reinterpret_cast<void *>(&ItemDisplayUpdateDetour), 6);

		Trace("item rarity native hooks installed chat=%d item_display=%d", chat_ok ? 1 : 0, item_ok ? 1 : 0);
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
