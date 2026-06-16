#include <Windows.h>
#include <cstring>

#include "dinput8.h"
#include "MQ2Main.h"
#include "core_init.h"

extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
extern "C" { __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; }

AddressLookupTable<void> ProxyAddressLookupTable = AddressLookupTable<void>();

DirectInput8CreateProc m_pDirectInput8Create = nullptr;
DllCanUnloadNowProc m_pDllCanUnloadNow = nullptr;
DllGetClassObjectProc m_pDllGetClassObject = nullptr;
DllRegisterServerProc m_pDllRegisterServer = nullptr;
DllUnregisterServerProc m_pDllUnregisterServer = nullptr;
GetdfDIJoystickProc m_pGetdfDIJoystick = nullptr;

namespace {
	HMODULE g_system_dinput8 = nullptr;
	bool g_native_initialized = false;

	void SetEverQuestPath()
	{
		char path[MAX_STRING] = { 0 };
		if (!GetModuleFileNameA(nullptr, path, MAX_STRING)) {
			gszEQPath[0] = 0;
			return;
		}

		char* last_slash = strrchr(path, '\\');
		if (last_slash) {
			*last_slash = 0;
		}

		strcpy_s(gszEQPath, path);
	}

	bool LoadSystemDirectInput()
	{
		char path[MAX_PATH] = { 0 };
		GetSystemDirectoryA(path, MAX_PATH);
		strcat_s(path, "\\dinput8.dll");

		g_system_dinput8 = LoadLibraryA(path);
		if (!g_system_dinput8) {
			return false;
		}

		m_pDirectInput8Create = reinterpret_cast<DirectInput8CreateProc>(GetProcAddress(g_system_dinput8, "DirectInput8Create"));
		m_pDllCanUnloadNow = reinterpret_cast<DllCanUnloadNowProc>(GetProcAddress(g_system_dinput8, "DllCanUnloadNow"));
		m_pDllGetClassObject = reinterpret_cast<DllGetClassObjectProc>(GetProcAddress(g_system_dinput8, "DllGetClassObject"));
		m_pDllRegisterServer = reinterpret_cast<DllRegisterServerProc>(GetProcAddress(g_system_dinput8, "DllRegisterServer"));
		m_pDllUnregisterServer = reinterpret_cast<DllUnregisterServerProc>(GetProcAddress(g_system_dinput8, "DllUnregisterServer"));
		m_pGetdfDIJoystick = reinterpret_cast<GetdfDIJoystickProc>(GetProcAddress(g_system_dinput8, "GetdfDIJoystick"));

		return m_pDirectInput8Create != nullptr;
	}

	void InitializeNativeAchievements(HMODULE module)
	{
		ghModule = module;
		ghInstance = reinterpret_cast<HINSTANCE>(module);
		SetEverQuestPath();

		if (!InitOffsets()) {
			return;
		}

		InitializeMQ2Detours();
		InitOptions();
		g_native_initialized = true;
	}

	void ShutdownNativeAchievements()
	{
		if (!g_native_initialized) {
			return;
		}

		ShutdownAchievementsNative();
		ShutdownMQ2Detours();
		g_native_initialized = false;
	}
}

bool WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(module);
		LoadSystemDirectInput();
		InitializeNativeAchievements(module);
		break;

	case DLL_PROCESS_DETACH:
		ShutdownNativeAchievements();
		if (g_system_dinput8) {
			FreeLibrary(g_system_dinput8);
			g_system_dinput8 = nullptr;
		}
		break;
	}

	return true;
}

HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* out, LPUNKNOWN outer)
{
	if (!m_pDirectInput8Create) {
		return E_FAIL;
	}

	HRESULT result = m_pDirectInput8Create(instance, version, iid, out, outer);
	if (SUCCEEDED(result)) {
		genericQueryInterface(iid, out);
	}

	return result;
}

HRESULT WINAPI DllCanUnloadNow()
{
	return m_pDllCanUnloadNow ? m_pDllCanUnloadNow() : E_FAIL;
}

HRESULT WINAPI DllGetClassObject(REFCLSID class_id, REFIID iid, LPVOID* out)
{
	if (!m_pDllGetClassObject) {
		return E_FAIL;
	}

	HRESULT result = m_pDllGetClassObject(class_id, iid, out);
	if (SUCCEEDED(result)) {
		genericQueryInterface(iid, out);
	}

	return result;
}

HRESULT WINAPI DllRegisterServer()
{
	return m_pDllRegisterServer ? m_pDllRegisterServer() : E_FAIL;
}

HRESULT WINAPI DllUnregisterServer()
{
	return m_pDllUnregisterServer ? m_pDllUnregisterServer() : E_FAIL;
}

LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
	return m_pGetdfDIJoystick ? m_pGetdfDIJoystick() : nullptr;
}
