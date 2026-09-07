#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include "MonsterSpawnInfo.h"
#include "../ODBC_Core/DBConnectionPool.h"

class CMonsterDBLoader
{
private:
	CDBConnectionPool*	m_connectionPool = nullptr;

public:
	CMonsterDBLoader() = default;
	~CMonsterDBLoader() = default;
	bool Initialize(CDBConnectionPool* _connectionPool);
	bool Load(std::vector<MONSTER_SPAWN_INFO>& _typeList);
};
