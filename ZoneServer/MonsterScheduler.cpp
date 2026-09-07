#include "MonsterScheduler.h"
#include "Channel.h"
#include "MonsterManager.h"

CMonsterScheduler::CMonsterScheduler() :
	m_channel(nullptr),
	m_monsterManager(nullptr),
	m_intervalMs(0),
	m_lastUpdateTime(0),
	m_nextUpdateTime(0),
	m_pending(false)
{
}

bool CMonsterScheduler::Initialize(CChannel* _channel, CMonsterManager* _monsterManager, uint64_t _intervalMs)
{
	if (_channel == nullptr || _monsterManager == nullptr || _intervalMs == 0) return false;
	if (m_channel != nullptr || m_monsterManager != nullptr) return false;

	m_channel = _channel;
	m_monsterManager = _monsterManager;
	m_intervalMs = _intervalMs;
	m_nextUpdateTime.store(GetTickCount64() + _intervalMs, std::memory_order_relaxed);
	return true;
}

bool CMonsterScheduler::TrySchedule(uint64_t _currentTime)
{
	if (m_channel == nullptr || m_monsterManager == nullptr) return false;
	if (_currentTime < m_nextUpdateTime.load(std::memory_order_relaxed)) return false;

	bool expected = false;
	return m_pending.compare_exchange_strong(expected, true, std::memory_order_relaxed);
}

void CMonsterScheduler::Execute()
{
	uint64_t currentTime = GetTickCount64();
	uint64_t deltaTimeMs = m_intervalMs;
	if (m_lastUpdateTime != 0) deltaTimeMs = currentTime - m_lastUpdateTime;

	m_lastUpdateTime = currentTime;
	m_nextUpdateTime.store(currentTime + m_intervalMs, std::memory_order_relaxed);

	std::vector<std::unique_ptr<CMonster>>& monsterList = m_monsterManager->GetList();
	uint64_t size = static_cast<uint64_t>(monsterList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CMonster* monster = monsterList[i].get();
		eMonsterState oldState = monster->GetState();
		uint64_t oldHP = monster->GetHP();
		uint64_t oldTarget = monster->GetAttackTargetObjectNum();
		CSector* oldSector = monster->GetSector();
		uint64_t oldPathVersion = monster->GetPathVersion();

		monster->Update(currentTime, deltaTimeMs, m_channel->GetPlayerManager());

		bool stateChanged = oldState != monster->GetState() || oldHP != monster->GetHP();
		bool targetChanged = oldTarget != monster->GetAttackTargetObjectNum() || monster->GetLastAttackDamage() > 0;
		// 섹터 또는 경로가 바뀐 경우에만 이동 정보 전송
		bool moveChanged = oldSector != monster->GetSector() || oldPathVersion != monster->GetPathVersion();
		if (stateChanged || targetChanged || moveChanged) m_channel->NotifyMonsterUpdated(monster);
	}

	m_pending.store(false, std::memory_order_relaxed);
}
