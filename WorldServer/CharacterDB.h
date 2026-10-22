#pragma once
#include <vector>
#include <string>
#include "../Common/CHARACTER.h"
#include "../Common/ZoneData.h"
#include "../ODBC_Core/DBConnectionPool.h"

class CCharacterDB
{
private:
	CDBConnectionPool*	m_connectionPool;

public:
	explicit CCharacterDB(CDBConnectionPool* _connectionPool) noexcept;
	~CCharacterDB() = default;

	bool	LoadList(uint64_t _accountIndex, std::vector<CHARACTER>& _characterList) const;
	bool	Create(uint64_t _accountIndex, eJobClass _class, const std::string& _name, const ZONE_INFO& _startZone, CHARACTER& _character) const;
	bool	Load(uint64_t _accountIndex, uint64_t _characterIndex, PLAYER_DATA& _data, std::string& _name) const;
};
