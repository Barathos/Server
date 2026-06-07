#ifndef __EQEMU_CRASH_H
#define __EQEMU_CRASH_H

#include <string>
#include <vector>

void set_exception_handler();

void PushCrashContext(const std::string& context);
void PopCrashContext();
std::vector<std::string> GetCrashContext();

class CrashContextScope {
public:
	explicit CrashContextScope(std::string context);
	~CrashContextScope();

	CrashContextScope(const CrashContextScope&) = delete;
	CrashContextScope& operator=(const CrashContextScope&) = delete;

private:
	bool active_ = false;
};

#endif
