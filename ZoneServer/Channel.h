#pragma once
#include <WinSock2.h>
#include <atomic>
#include <cstdint>
#include <string>
#include <functional>
#include "Field.h"
#include "PlayerManager.h"
#include "MonsterManager.h"
#include "MonsterScheduler.h"
#include "../Utility_Core/Job.h"
#include "../Utility_Core/MPSCJobExecutor.h"

class CPlayerStatDB;

class CChannel
{
private:
	uint64_t									m_channelId;
	CField										m_field;
	CPlayerManager								m_playerManager;
	CMonsterManager								m_monsterManager;
	CMPSCJobExecutor							m_jobExecutor;
	CMonsterScheduler							m_monsterScheduler;
	std::atomic_bool							m_active;
	std::atomic<uint64_t>						m_playerCount;
	std::atomic<uint64_t>						m_reservedPlayerCount;
	uint64_t									m_maxPlayerCount;
	std::function<void(CChannel*, CMonster*)>	m_monsterUpdateHandler;

public:
	CChannel();
	~CChannel();

	bool Initialize(uint64_t _channelId, uint64_t _playerPoolSize, const std::string& _fieldInfoPath, const std::vector<MONSTER_SPAWN_INFO>* _spawnInfoList = nullptr, const CPlayerStatDB* _playerStatDB = nullptr);
	bool Start();
	void Stop();
	void PushJob(CJob* _job);
	void ScheduleMonster(uint64_t _currentTime);
	bool IsActive();
	void SetMonsterUpdateHandler(const std::function<void(CChannel*, CMonster*)>& _handler);
	void NotifyMonsterUpdated(CMonster* _monster);

	CPlayer* CreatePlayer(uint64_t _characterIndex, const std::string& _name, eClassState _class, const VECTOR3& _position, uint64_t _level = 1, uint64_t _experience = 0);
	CPlayer* CreateReservedPlayer(uint64_t _characterIndex, const std::string& _name, eClassState _class, const VECTOR3& _position, uint64_t _level, uint64_t _experience);
	bool ReservePlayer();
	void CancelPlayerReservation();
	void RemovePlayer(CPlayer* _player);
	bool MovePlayer(CPlayer* _player, const VECTOR3& _position);

	uint64_t GetChannelId();
	uint64_t GetPlayerCount();
	uint64_t GetMaxPlayerCount();
	uint64_t GetReservedPlayerCount();
	CField* GetField();
	CPlayerManager* GetPlayerManager();
	CMonsterManager* GetMonsterManager();
};
