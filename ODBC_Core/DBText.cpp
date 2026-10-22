#include "DBText.h"

std::wstring CDBText::EscapeSQL(const std::wstring& _text)
{
	std::wstring result;
	result.reserve(_text.size());

	for (uint64_t i = 0; i < _text.size(); ++i)
	{
		if (_text[i] == L'\\') result.push_back(L'\\');
		if (_text[i] == L'\'') result.push_back(L'\'');
		result.push_back(_text[i]);
	}

	return result;
}

std::wstring CDBText::UTF8ToWide(const char* _text, int _length)
{
	if (_text == nullptr || _length <= 0) return std::wstring();

	int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, _text, _length, nullptr, 0);
	if (wideLength <= 0) return std::wstring();

	std::wstring result(wideLength, L'\0');
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, _text, _length, result.data(), wideLength) != wideLength)
	{
		return std::wstring();
	}

	return result;
}
