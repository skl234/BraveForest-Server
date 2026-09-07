#include "Timer.h"
#include <chrono>

std::string CTimer::GetTimeLog()
{
	//나노초 단위
	auto now = std::chrono::system_clock::now();

	//초단위, 이걸 따로 구하는 이유는 
	//(YYYY-MM-DD HH:MM:SS) 이걸 쉽게 구하기 위해
	auto time = std::chrono::system_clock::to_time_t(now);

	//현재시간을 밀리초단위로 바꾼다음, 초단위의 밀리초만 구함
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm localTime{};
	localtime_s(&localTime, &time);

	char buffer[64];

	std::snprintf(
		buffer,
		sizeof(buffer),
		"[%04d-%02d-%02d %02d:%02d:%02d.%03lld]",
		localTime.tm_year + 1900,
		localTime.tm_mon + 1,
		localTime.tm_mday,
		localTime.tm_hour,
		localTime.tm_min,
		localTime.tm_sec,
		static_cast<long long>(ms.count())
	);

	return std::string(buffer);
}

std::wstring CTimer::GetTimeLogW()
{
	//현재 시간 (나노초 단위)
	auto now = std::chrono::system_clock::now();

	//초 단위로 변환
	auto time = std::chrono::system_clock::to_time_t(now);

	//밀리초
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm localTime{};
	localtime_s(&localTime, &time);

	wchar_t buffer[64];

	swprintf(
		buffer,
		sizeof(buffer) / sizeof(wchar_t),
		L"[%04d-%02d-%02d %02d:%02d:%02d.%03lld]",
		localTime.tm_year + 1900,
		localTime.tm_mon + 1,
		localTime.tm_mday,
		localTime.tm_hour,
		localTime.tm_min,
		localTime.tm_sec,
		static_cast<long long>(ms.count())
	);

	return std::wstring(buffer);
}

int64_t CTimer::GetCurrentTick()
{
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()
		).count();

	return ms;
}
