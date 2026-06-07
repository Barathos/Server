#include "global_define.h"
#include "eqemu_logsys.h"
#include "crash.h"
#include "strings.h"
#include "process/process.h"
#include "http/httplib.h"
#include "http/uri.h"
#include "json/json.h"
#include "version.h"
#include "eqemu_config.h"
#include "serverinfo.h"
#include "rulesys.h"
#include "platform.h"

#include <cstdio>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <vector>

#ifdef _WINDOWS
#define popen _popen
#endif

namespace {
	thread_local std::vector<std::string> crash_context;
}

void PushCrashContext(const std::string& context)
{
	if (!context.empty()) {
		crash_context.push_back(context);
	}
}

void PopCrashContext()
{
	if (!crash_context.empty()) {
		crash_context.pop_back();
	}
}

std::vector<std::string> GetCrashContext()
{
	return crash_context;
}

CrashContextScope::CrashContextScope(std::string context)
{
	if (!context.empty()) {
		PushCrashContext(context);
		active_ = true;
	}
}

CrashContextScope::~CrashContextScope()
{
	if (active_) {
		PopCrashContext();
	}
}

void SendCrashReport(const std::string &crash_report)
{
	// can configure multiple endpoints if need be
	std::vector<std::string> endpoints = {
		"https://spire.akkadius.com/api/v1/analytics/server-crash-report",
//		"http://localhost:3010/api/v1/analytics/server-crash-report", // development
	};

	EQEmuLogSys* log = EQEmuLogSys::Instance();

	auto      config = EQEmuConfig::get();
	for (auto &e: endpoints) {
		uri u(e);

		std::string base_url = fmt::format("{}://{}", u.get_scheme(), u.get_host());
		if (u.get_port()) {
			base_url += fmt::format(":{}", u.get_port());
		}

		// client
		httplib::Client r(base_url);
		r.set_connection_timeout(1, 0);
		r.set_read_timeout(1, 0);
		r.set_write_timeout(1, 0);

		// os info
		auto os         = EQ::GetOS();
		auto cpus       = EQ::GetCPUs();
		auto process_id = EQ::GetPID();
		auto rss        = EQ::GetRSS() / 1048576.0;
		auto uptime     = static_cast<uint32>(EQ::GetUptime());

		// payload
		Json::Value p;
		p["platform_name"]     = GetPlatformName();
		p["crash_report"]      = crash_report;
		p["server_version"]    = CURRENT_VERSION;
		p["compile_date"]      = COMPILE_DATE;
		p["compile_time"]      = COMPILE_TIME;
		p["server_name"]       = config->LongName;
		p["server_short_name"] = config->ShortName;
		p["uptime"]            = uptime;
		p["os_machine"]        = os.machine;
		p["os_release"]        = os.release;
		p["os_version"]        = os.version;
		p["os_sysname"]        = os.sysname;
		p["process_id"]        = process_id;
		p["rss_memory"]        = rss;
		p["cpus"]              = cpus.size();
		p["origination_info"]  = "";

		if (!log->origination_info.zone_short_name.empty()) {
			p["origination_info"] = fmt::format(
				"{} ({}) instance_id [{}]",
				log->origination_info.zone_short_name,
				log->origination_info.zone_long_name,
				log->origination_info.instance_id
			);
		}

		std::stringstream payload;
		payload << p;

		if (auto res = r.Post(e, payload.str(), "application/json")) {
			if (res->status == 200) {
				LogInfo("Sent crash report");
			}
			else {
				LogError("Failed to send crash report to [{}]", e);
			}
		}
	}
}

#if defined(_WINDOWS) && defined(CRASH_LOGGING)
#include <dbghelp.h>
#include "StackWalker.h"

class EQEmuStackWalker : public StackWalker
{
public:
	EQEmuStackWalker() : StackWalker() { }
	EQEmuStackWalker(DWORD dwProcessId, HANDLE hProcess) : StackWalker(dwProcessId, hProcess) { }
	virtual void OnOutput(LPCSTR szText) {
		char buffer[4096];
		for (int i = 0; i < 4096; ++i) {
			if (szText[i] == 0) {
				buffer[i] = '\0';
				break;
			}

			if (szText[i] == '\n' || szText[i] == '\r') {
				buffer[i] = ' ';
			}
			else {
				buffer[i] = szText[i];
			}
		}

		std::string line = buffer;
		_lines.push_back(line);

		Log(Logs::General, Logs::Crash, buffer);
		StackWalker::OnOutput(szText);
	}

	const std::vector<std::string>& GetLines() { return _lines; }
private:
	std::vector<std::string> _lines;
};

static std::string GetCrashDumpPath()
{
	char module_path[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, module_path, MAX_PATH);

	std::filesystem::path dump_dir = std::filesystem::current_path() / "crashdumps";
	if (module_path[0] != '\0') {
		const std::filesystem::path exe_path(module_path);
		const auto exe_dir = exe_path.parent_path();
		if (exe_dir.filename() == "bin") {
			dump_dir = exe_dir.parent_path() / "crashdumps";
		}
	}

	std::error_code error;
	std::filesystem::create_directories(dump_dir, error);

	SYSTEMTIME system_time;
	GetLocalTime(&system_time);

	auto process_name = GetPlatformName();
	for (auto& character : process_name) {
		if (!std::isalnum(static_cast<unsigned char>(character))) {
			character = '_';
		}
	}

	const auto dump_name = fmt::format(
		"{}_pid{}_{}{:02}{:02}_{:02}{:02}{:02}_{:03}.dmp",
		process_name,
		GetCurrentProcessId(),
		system_time.wYear,
		system_time.wMonth,
		system_time.wDay,
		system_time.wHour,
		system_time.wMinute,
		system_time.wSecond,
		system_time.wMilliseconds
	);

	return (dump_dir / dump_name).string();
}

static void WriteCrashDump(EXCEPTION_POINTERS* exception_info)
{
	const auto dbghelp = LoadLibraryA("dbghelp.dll");
	if (!dbghelp) {
		Log(Logs::General, Logs::Crash, "Crash dump: failed to load dbghelp.dll");
		return;
	}

	using MiniDumpWriteDumpFunction = BOOL (WINAPI *)(
		HANDLE,
		DWORD,
		HANDLE,
		MINIDUMP_TYPE,
		PMINIDUMP_EXCEPTION_INFORMATION,
		PMINIDUMP_USER_STREAM_INFORMATION,
		PMINIDUMP_CALLBACK_INFORMATION
	);

	const auto mini_dump_write_dump = reinterpret_cast<MiniDumpWriteDumpFunction>(
		GetProcAddress(dbghelp, "MiniDumpWriteDump")
	);
	if (!mini_dump_write_dump) {
		Log(Logs::General, Logs::Crash, "Crash dump: MiniDumpWriteDump was not available");
		FreeLibrary(dbghelp);
		return;
	}

	const auto dump_path = GetCrashDumpPath();
	const auto dump_file = CreateFileA(
		dump_path.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (dump_file == INVALID_HANDLE_VALUE) {
		const auto line = fmt::format("Crash dump: failed to create [{}] error [{}]", dump_path, GetLastError());
		Log(Logs::General, Logs::Crash, line.c_str());
		FreeLibrary(dbghelp);
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION dump_exception_info = {};
	dump_exception_info.ThreadId = GetCurrentThreadId();
	dump_exception_info.ExceptionPointers = exception_info;
	dump_exception_info.ClientPointers = FALSE;

	const auto dump_type = static_cast<MINIDUMP_TYPE>(
		MiniDumpWithFullMemory |
		MiniDumpWithHandleData |
		MiniDumpWithThreadInfo |
		MiniDumpWithUnloadedModules
	);

	const auto wrote_dump = mini_dump_write_dump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		dump_file,
		dump_type,
		&dump_exception_info,
		nullptr,
		nullptr
	);

	CloseHandle(dump_file);
	FreeLibrary(dbghelp);

	if (wrote_dump) {
		const auto line = fmt::format("Crash dump written: [{}]", dump_path);
		Log(Logs::General, Logs::Crash, line.c_str());
	} else {
		const auto line = fmt::format("Crash dump: MiniDumpWriteDump failed for [{}] error [{}]", dump_path, GetLastError());
		Log(Logs::General, Logs::Crash, line.c_str());
	}
}

LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
	switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
			Log(Logs::General, Logs::Crash, "EXCEPTION_ACCESS_VIOLATION");
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			Log(Logs::General, Logs::Crash, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
			break;
		case EXCEPTION_BREAKPOINT:
			Log(Logs::General, Logs::Crash, "EXCEPTION_BREAKPOINT");
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			Log(Logs::General, Logs::Crash, "EXCEPTION_DATATYPE_MISALIGNMENT");
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_DENORMAL_OPERAND");
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_DIVIDE_BY_ZERO");
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_INEXACT_RESULT");
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_INVALID_OPERATION");
			break;
		case EXCEPTION_FLT_OVERFLOW:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_OVERFLOW");
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_STACK_CHECK");
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			Log(Logs::General, Logs::Crash, "EXCEPTION_FLT_UNDERFLOW");
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			Log(Logs::General, Logs::Crash, "EXCEPTION_ILLEGAL_INSTRUCTION");
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			Log(Logs::General, Logs::Crash, "EXCEPTION_IN_PAGE_ERROR");
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			Log(Logs::General, Logs::Crash, "EXCEPTION_INT_DIVIDE_BY_ZERO");
			break;
		case EXCEPTION_INT_OVERFLOW:
			Log(Logs::General, Logs::Crash, "EXCEPTION_INT_OVERFLOW");
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			Log(Logs::General, Logs::Crash, "EXCEPTION_INVALID_DISPOSITION");
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			Log(Logs::General, Logs::Crash, "EXCEPTION_NONCONTINUABLE_EXCEPTION");
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			Log(Logs::General, Logs::Crash, "EXCEPTION_PRIV_INSTRUCTION");
			break;
		case EXCEPTION_SINGLE_STEP:
			Log(Logs::General, Logs::Crash, "EXCEPTION_SINGLE_STEP");
			break;
		case EXCEPTION_STACK_OVERFLOW:
			Log(Logs::General, Logs::Crash, "EXCEPTION_STACK_OVERFLOW");
			break;
		default:
			Log(Logs::General, Logs::Crash, "Unknown Exception");
			break;
	}

	{
		const auto record = ExceptionInfo->ExceptionRecord;
		const auto exception_address = reinterpret_cast<uintptr_t>(record->ExceptionAddress);
		const auto line = fmt::format(
			"Exception detail: code [0x{:08X}] address [0x{:016X}] flags [0x{:08X}] parameters [{}]",
			static_cast<uint32>(record->ExceptionCode),
			static_cast<uint64>(exception_address),
			static_cast<uint32>(record->ExceptionFlags),
			static_cast<uint32>(record->NumberParameters)
		);
		Log(Logs::General, Logs::Crash, line.c_str());

		if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
			const auto operation = record->ExceptionInformation[0];
			const auto address = record->ExceptionInformation[1];
			const char* operation_name = "unknown";
			if (operation == 0) {
				operation_name = "read";
			} else if (operation == 1) {
				operation_name = "write";
			} else if (operation == 8) {
				operation_name = "execute";
			}

			const auto access_line = fmt::format(
				"Access violation detail: attempted [{}] at address [0x{:016X}]",
				operation_name,
				static_cast<uint64>(address)
			);
			Log(Logs::General, Logs::Crash, access_line.c_str());
		}
	}

	const auto context = GetCrashContext();
	if (!context.empty()) {
		Log(Logs::General, Logs::Crash, "Crash context:");
		for (size_t i = 0; i < context.size(); ++i) {
			const auto line = fmt::format("  [{}] {}", i, context[i]);
			Log(Logs::General, Logs::Crash, line.c_str());
		}
	}

	WriteCrashDump(ExceptionInfo);

	if(EXCEPTION_STACK_OVERFLOW != ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		EQEmuStackWalker sw;
		sw.ShowCallstack(GetCurrentThread(), ExceptionInfo->ContextRecord);

		if (RuleB(Analytics, CrashReporting)) {
			std::string crash_report;
			auto& lines = sw.GetLines();

			for (auto& line : lines) {
				crash_report += line;
				crash_report += "\n";
			}

			SendCrashReport(crash_report);
		}
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void set_exception_handler() {
	SetUnhandledExceptionFilter(windows_exception_handler);
}
#else

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/fcntl.h>

#ifdef __FreeBSD__
#include <signal.h>
#include <sys/stat.h>
#endif

void print_trace()
{
	bool does_gdb_exist = Strings::Contains(Process::execute("gdb -v"), "GNU");
	if (!does_gdb_exist) {
		LogCrash(
			"[Error] GDB is not installed, if you want crash dumps on Linux to work properly you will need GDB installed"
		);
		std::exit(1);
	}

	auto        uid              = geteuid();
	std::string temp_output_file = fmt::format("/tmp/dump-output-{}", Strings::Random(10));

	// check for passwordless sudo if not root
	if (uid != 0) {
		bool sudo_password_required = Strings::Contains(Process::execute("sudo -n true"), "a password is required");
		if (sudo_password_required) {
			LogCrash(
				"[Error] Current user does not have passwordless sudo installed. It is required to automatically process crash dumps with GDB as non-root."
			);
			std::exit(1);
		}
	}

	char pid_buf[30];
	sprintf(pid_buf, "%d", getpid());
	char name_buf[512];
	name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;
	int child_pid = fork();
	if (!child_pid) {
		int fd = open(temp_output_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		dup2(fd, 1); // redirect output to stderr
		fprintf(stdout, "stack trace for %s pid=%s\n", name_buf, pid_buf);
		if (uid == 0) {
			execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
		}
		else {
			execlp("sudo", "gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
		}

		close(fd);

		abort(); /* If gdb failed to start */
	}
	else {
		waitpid(child_pid, nullptr, 0);
	}

	std::ifstream    input(temp_output_file);
	std::string      crash_report;
	for (std::string line; getline(input, line);) {
		LogCrash("{}", line);
		crash_report += fmt::format("{}\n", line);
	}

	std::remove(temp_output_file.c_str());

	if (RuleB(Analytics, CrashReporting)) {
		SendCrashReport(crash_report);
	}

	EQEmuLogSys::Instance()->CloseFileLogs();

	exit(1);
}

// crash is off or an unhandled platform
void set_exception_handler()
{
	signal(SIGABRT, reinterpret_cast<void (*)(int)>(print_trace));
	signal(SIGFPE, reinterpret_cast<void (*)(int)>(print_trace));
	signal(SIGFPE, reinterpret_cast<void (*)(int)>(print_trace));
	signal(SIGSEGV, reinterpret_cast<void (*)(int)>(print_trace));
}
#endif
