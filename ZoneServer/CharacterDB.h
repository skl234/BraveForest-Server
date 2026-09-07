#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>
#include "../Common/ZONE.h"
#include "../ODBC_Core/DBConnectionPool.h"

class CCharacterDB
{
private:
	CDBConnectionPool*	m_connectionPool = nullptr;

public:
	CCharacterDB() = default;
	~CCharacterDB() = default;

	bool Initialize(CDBConnectionPool* _connectionPool);
	bool Load(uint64_t _accountIndex, uint64_t _characterIndex, PLAYER_DATA& _playerData, std::string& _name);
	bool Save(const PLAYER_DATA& _playerData);
};
