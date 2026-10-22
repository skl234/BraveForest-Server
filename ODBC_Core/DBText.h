#pragma once
#include <Windows.h>
#include <string>

class CDBText
{
public:
	static std::wstring EscapeSQL(const std::wstring& _text);
	static std::wstring UTF8ToWide(const char* _text, int _length);
};
