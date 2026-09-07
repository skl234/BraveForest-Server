#define _CRT_SECURE_NO_WARNINGS

#include "LogManager.h"
#include <filesystem>
#include "LockGuard.h"
#include "Timer.h"

CLogManager::CLogManager() :
	m_levelList{ "DEBUG", "INFO", "WARN", "ERROR" },
	m_wLevelList{ L"DEBUG", L"INFO", L"WARN", L"ERROR" },
	m_file(nullptr)
{
	InitializeSRWLock(&m_lock);
}

CLogManager::~CLogManager()
{
	if (m_file) fclose(m_file);
}

bool CLogManager::Initalize(const std::string& _path)
{
	m_file = fopen(_path.data(), "a");
	if (!m_file) return false;

	return true;
}

bool CLogManager::Write(const eLogLevel& _logLevel, const char* _format, ...)
{
	if (!m_file) return false;

	std::string& level = m_levelList[static_cast<byte>(_logLevel)];
	std::string time = CTimer::GetTimeLog();

	char log[1024];
	va_list args;
	va_start(args, _format);
	vsnprintf(log, sizeof(log), _format, args);
	va_end(args);

	CLockGuard guard(m_lock);
	fprintf(m_file, "[%s] [%s] %s\n", level.data(), time.data(), log);
	fflush(m_file);

	return true;
}

bool CLogManager::WriteW(const eLogLevel& _logLevel, const wchar_t* _format, ...)
{
	if (!m_file) return false;

	std::wstring& level = m_wLevelList[static_cast<byte>(_logLevel)];
	std::wstring time = CTimer::GetTimeLogW();

	wchar_t log[1024];
	va_list args;
	va_start(args, _format);
	vswprintf(log, sizeof(log) / sizeof(wchar_t), _format, args);
	va_end(args);

	CLockGuard guard(m_lock);
	fwprintf(m_file, L"[%ls] [%ls] %ls\n", level.data(), time.data(), log);
	fflush(m_file);

	return true;
}
