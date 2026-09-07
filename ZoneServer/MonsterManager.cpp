#include "MonsterManager.h"

bool CMonsterManager::Initialize(const std::vector<MONSTER_SPAWN_INFO>& _spawnInfoList)
{
	if (!m_list.empty()) return false;

	uint64_t size = static_cast<uint64_t>(_spawnInfoList.size());
	m_list.reserve(size);

	for (uint64_t i = 0; i < size; ++i)
	{
		std::unique_ptr<CMonster> monster =
			std::make_unique<CMonster>(i, _spawnInfoList[i]);
		m_list.emplace_back(std::move(monster));
	}
	return true;
}

CMonster* CMonsterManager::Find(uint64_t _objectNum)
{
	if (_objectNum >= m_list.size()) return nullptr;
	return m_list[_objectNum].get();
}

std::vector<std::unique_ptr<CMonster>>& CMonsterManager::GetList()
{
	return m_list;
}

uint64_t CMonsterManager::GetSize()
{
	return static_cast<uint64_t>(m_list.size());
}
