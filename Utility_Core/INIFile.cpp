#include "INIFile.h"
#include <Windows.h>
#include <iostream>
#include <vector>

bool CINIFile::Open(const std::filesystem::path& _path)
{
	m_path.clear();
	std::error_code error;
	if (!std::filesystem::is_regular_file(_path, error))
	{
		std::cerr << "INI file not found : " << _path.string() << "\n";
		return false;
	}

	m_path = std::filesystem::absolute(_path, error).lexically_normal();
	if (error)
	{
		m_path.clear();
		return false;
	}
	return true;
}

std::wstring CINIFile::ReadString(const wchar_t* _section, const wchar_t* _key, const wchar_t* _default)
{
	if (m_path.empty()) return _default;

	std::vector<wchar_t> buffer(512);
	while (true)
	{
		DWORD size = GetPrivateProfileStringW(_section, _key, _default, buffer.data(), static_cast<DWORD>(buffer.size()), m_path.c_str());
		if (size < buffer.size() - 1) return std::wstring(buffer.data(), size);
		buffer.resize(buffer.size() * 2);
	}
}

uint64_t CINIFile::ReadNumber(const wchar_t* _section, const wchar_t* _key, uint32_t _default)
{
	if (m_path.empty()) return _default;
	return GetPrivateProfileIntW(_section, _key, static_cast<INT>(_default), m_path.c_str());
}

bool CINIFile::ReadPath(const wchar_t* _section, const wchar_t* _key, std::string& _path)
{
	_path.clear();
	std::wstring value = ReadString(_section, _key);
	if (value.empty()) return false;

	// 상대경로는 INI 위치 기준
	std::filesystem::path path(value);
	if (path.is_relative()) path = m_path.parent_path() / path;
	path = path.lexically_normal();
	std::error_code error;
	if (!std::filesystem::is_regular_file(path, error))
	{
		std::cerr << "Data file not found : " << path.string() << "\n";
		return false;
	}
	_path = path.string();
	return true;
}

std::filesystem::path CINIFile::GetExecutableDirectory()
{
	wchar_t path[MAX_PATH]{};
	DWORD size = GetModuleFileNameW(nullptr, path, MAX_PATH);
	if (size == 0 || size >= MAX_PATH) return {};
	return std::filesystem::path(path).parent_path();
}
