#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include "ClassType.h"
#include "../ODBC_Core/DBConnectionPool.h"

struct PLAYER_LEVEL_STAT
{
	eClassState	jobClass;
	uint64_t	level;
	uint64_t	requiredExperience;
	uint64_t	maxHP;
};

class CPlayerStatDB
{
private:
	CDBConnectionPool*				m_connectionPool = nullptr;
	std::vector<PLAYER_LEVEL_STAT>	m_statList;

public:
	CPlayerStatDB() = default;
	~CPlayerStatDB() = default;

	bool Initialize(CDBConnectionPool* _connectionPool);
	bool Load();
	const PLAYER_LEVEL_STAT* Find(eClassState _class, uint64_t _level) const
	{
		if (_level == 0 || _level > 20) return nullptr;
		uint64_t jobClass = static_cast<uint64_t>(_class);
		if (jobClass > static_cast<uint64_t>(eClassState::Archer)) return nullptr;
		uint64_t index = jobClass * 21 + _level;
		if (index >= m_statList.size() || m_statList[index].level == 0) return nullptr;
		return &m_statList[index];
	}
};
