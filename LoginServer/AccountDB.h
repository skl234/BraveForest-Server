#pragma once
#include <cstdint>
#include <string>

class CAccountDB
{
public:
	static bool Login(const std::wstring& _id, const std::wstring& _password, uint64_t& _accountIndex);
	static bool Logout(uint64_t _accountIndex);
	static bool Create(const std::wstring& _id, const std::wstring& _password);
};
