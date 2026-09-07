#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "FieldObject.h"
#include "MonsterState.h"
#include "MonsterSpawnInfo.h"
#include "Navigator.h"

class CPlayer;
class CPlayerManager;

class CMonster : public CFieldObject
{
private:
	eMonsterType			m_type;
	eMonsterState			m_state;
	uint64_t				m_level;
	uint64_t				m_maxHP;
	uint64_t				m_hp;
	uint64_t				m_experience;
	uint64_t				m_attackPower;
	bool					m_aggressive;
	bool					m_canWander;
	bool					m_canChase;
	bool					m_canAttack;
	bool					m_invincible;
	float					m_detectionRange;
	float					m_attackRange;
	float					m_moveSpeed;
	uint64_t				m_attackIntervalMs;
	uint64_t				m_idleMinTimeMs;
	uint64_t				m_idleMaxTimeMs;
	uint64_t				m_wanderMinTimeMs;
	uint64_t				m_wanderMaxTimeMs;
	VECTOR3					m_spawnPosition;
	VECTOR3					m_destination;
	float					m_wanderRadius;
	float					m_maxAggroRange;
	float					m_respawnRadius;
	uint64_t				m_respawnDelayMs;
	uint64_t				m_nextAttackTime;
	uint64_t				m_respawnTime;
	uint64_t				m_nextWanderTime;
	uint64_t				m_wanderEndTime;
	uint64_t				m_lastUpdateTime;
	uint64_t				m_targetCellIndex;
	uint64_t				m_pathIndex;
	uint64_t				m_pathVersion;
	uint64_t				m_sentPathVersion;
	std::vector<VECTOR3>	m_path;
	CNavigator				m_navigator;
	uint64_t				m_attackTargetObjectNum;
	uint64_t				m_lastAttackDamage;

public:
	CMonster(uint64_t _objectNum, const MONSTER_SPAWN_INFO& _spawnInfo);
	~CMonster() override = default;
	void Update(uint64_t _currentTime, uint64_t _deltaTimeMs, CPlayerManager* _playerManager);
	uint64_t TakeDamage(uint64_t _damage);
	uint64_t TakeDamage(CPlayer* _attacker, uint64_t _damage, uint64_t _currentTime);
	void ReleaseTarget(uint64_t _playerObjectNum);
	eMonsterType GetType();
	eMonsterState GetState();
	uint64_t GetLevel();
	uint64_t GetMaxHP();
	uint64_t GetHP();
	uint64_t GetExperience();
	uint64_t GetAttackPower();
	bool IsAggressive();
	bool IsInvincible();
	uint64_t GetAttackTargetObjectNum();
	uint64_t GetLastAttackDamage();
	uint64_t ConsumeAttackDamage();
	bool ConsumePathChanged();
	VECTOR3& GetSpawnPosition();
	VECTOR3& GetDestination();
	float GetWanderRadius();
	float GetMaxAggroRange();
	float GetRespawnRadius();
	uint64_t GetRespawnDelayMs();
	float GetMoveSpeed();
	uint64_t GetPathIndex();
	uint64_t GetPathVersion();
	std::vector<VECTOR3>& GetPath();

private:
	CPlayer* FindNearbyPlayer();
	void UpdateIdle(uint64_t _currentTime, uint64_t _deltaTimeMs, CPlayer* _target);
	void UpdateChase(uint64_t _currentTime, uint64_t _deltaTimeMs, CPlayer* _target);
	void UpdateAttack(uint64_t _currentTime, CPlayer* _target);
	void UpdateReturn(uint64_t _deltaTimeMs);
	void UpdateDead(uint64_t _currentTime);
	bool CreatePath(const VECTOR3& _destination);
	bool CreateChasePath(const VECTOR3& _targetPosition);
	bool MovePath(uint64_t _deltaTimeMs);
	uint64_t GetRandomTime(uint64_t _minTime, uint64_t _maxTime, uint64_t _currentTime);
	void ClearPath();
	void StartReturn();
	void Die(uint64_t _currentTime);
};
