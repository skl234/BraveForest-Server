#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "Monster.h"
#include "MonsterSpawnInfo.h"

class CMonsterManager
{
private:
	std::vector<std::unique_ptr<CMonster>>	m_list;

public:
	CMonsterManager() = default;
	~CMonsterManager() = default;

	bool Initialize(const std::vector<MONSTER_SPAWN_INFO>& _spawnInfoList);
	CMonster* Find(uint64_t _objectNum);

	std::vector<std::unique_ptr<CMonster>>& GetList();
	uint64_t GetSize();
};
