#pragma once
#include "MonsterSpawnInfo.h"
#include <vector>
#include <string>

class CMonsterSpawnData
{
public:
	bool	Load(const std::string& _path, uint64_t _zoneId, const std::vector<MONSTER_SPAWN_INFO>& _typeList, std::vector<MONSTER_SPAWN_INFO>& _spawnList);

private:
	bool	Error(const std::string& _path, const char* _reason);
};
