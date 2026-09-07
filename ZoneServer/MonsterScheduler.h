#pragma once
#include <WinSock2.h>
#include <atomic>
#include <cstdint>
#include "../Utility_Core/Job.h"

class CChannel;
class CMonsterManager;

class CMonsterScheduler : public CJob
{
private:
	CChannel*				m_channel;
	CMonsterManager*		m_monsterManager;
	uint64_t				m_intervalMs;
	uint64_t				m_lastUpdateTime;
	std::atomic<uint64_t>	m_nextUpdateTime;
	std::atomic_bool		m_pending;

public:
	CMonsterScheduler();
	~CMonsterScheduler() override = default;

	bool Initialize(CChannel* _channel, CMonsterManager* _monsterManager, uint64_t _intervalMs);
	bool TrySchedule(uint64_t _currentTime);
	void Execute() override;
};
