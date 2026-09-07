#pragma once
#include <cstdint>
#include <string>
#include <filesystem>

class CINIFile
{
private:
	std::filesystem::path	m_path;

public:
	bool Open(const std::filesystem::path& _path);
	std::wstring ReadString(const wchar_t* _section, const wchar_t* _key, const wchar_t* _default = L"");
	uint64_t ReadNumber(const wchar_t* _section, const wchar_t* _key, uint32_t _default = 0);
	bool ReadPath(const wchar_t* _section, const wchar_t* _key, std::string& _path);

	static std::filesystem::path GetExecutableDirectory();
};
