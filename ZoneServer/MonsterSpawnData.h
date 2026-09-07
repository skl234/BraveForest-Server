#pragma once
#include "MonsterSpawnInfo.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cmath>

class CMonsterSpawnData
{
public:
	bool Load(const std::string& _path, uint64_t _zoneId, const std::vector<MONSTER_SPAWN_INFO>& _typeList, std::vector<MONSTER_SPAWN_INFO>& _spawnList)
	{
		_spawnList.clear();
		std::ifstream file(_path);
		if (!file.is_open()) return Error(_path, "file open");
		if (file.peek() == 0xEF) file.ignore(3);
		std::string name;
		uint64_t zoneId = 0;
		uint64_t count = 0;
		if (!(file >> name >> zoneId) || name != "ZoneId" || zoneId != _zoneId) return Error(_path, "ZoneId");
		if (!(file >> name >> count) || name != "SpawnCount" || count > 512) return Error(_path, "SpawnCount");
		std::vector<MONSTER_SPAWN_INFO> list;
		for (uint64_t i = 0; i < count; ++i)
		{
			uint64_t index = 0;
			uint64_t type = 0;
			VECTOR3 position{};
			float rotationY = 0.0f;
			if (!(file >> name >> index >> type >> position.x >> position.z >> rotationY)) return Error(_path, "Spawn record");
			if (name != "Spawn" || index != i) return Error(_path, "spawn index");
			if (type >= _typeList.size() || _typeList[type].level == 0) return Error(_path, "monster type");
			if (!std::isfinite(position.x) || !std::isfinite(position.z) || !std::isfinite(rotationY) || position.x < 0.0f || position.z < 0.0f)
				return Error(_path, "spawn position/rotation");
			MONSTER_SPAWN_INFO info = _typeList[type];
			info.spawnPosition = position;
			info.rotationY = rotationY;
			list.push_back(info);
		}
		if (file >> name) return Error(_path, "unexpected trailing data");
		_spawnList.swap(list);
		return true;
	}

private:
	bool Error(const std::string& _path, const char* _reason)
	{
		std::cerr << "[MonsterSpawnData] " << _path << " : " << _reason << "\n";
		return false;
	}
};
