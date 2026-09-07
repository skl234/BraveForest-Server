#pragma once
#include <cstdint>
#include <string>

class CTimer
{
public:
	static std::string GetTimeLog();
	static std::wstring GetTimeLogW();
	static int64_t GetCurrentTick(); //milliseconds
};
