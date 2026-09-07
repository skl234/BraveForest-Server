#include "Monster.h"
#include "Player.h"
#include "PlayerManager.h"
#include "Sector.h"
#include "Field.h"
#include "CombatDamageCalculator.h"
#include "../Utility_Core/RandomNumberGenerator.h"
#include <set>
#include <vector>
#include <cmath>

CMonster::CMonster(uint64_t _objectNum, const MONSTER_SPAWN_INFO& _spawnInfo) :
	CFieldObject(eFieldObjectType::Monster, _objectNum),
	m_type(_spawnInfo.type),
	m_state(eMonsterState::Idle),
	m_level(_spawnInfo.level),
	m_maxHP(_spawnInfo.maxHP),
	m_hp(_spawnInfo.maxHP),
	m_experience(_spawnInfo.experience),
	m_attackPower(_spawnInfo.attackPower),
	m_aggressive(_spawnInfo.aggressive),
	m_canWander(_spawnInfo.canWander),
	m_canChase(_spawnInfo.canChase),
	m_canAttack(_spawnInfo.canAttack),
	m_invincible(false),
	m_detectionRange(_spawnInfo.detectionRange),
	m_attackRange(_spawnInfo.attackRange),
	m_moveSpeed(_spawnInfo.moveSpeed),
	m_attackIntervalMs(_spawnInfo.attackIntervalMs),
	m_idleMinTimeMs(_spawnInfo.idleMinTimeMs),
	m_idleMaxTimeMs(_spawnInfo.idleMaxTimeMs),
	m_wanderMinTimeMs(_spawnInfo.wanderMinTimeMs),
	m_wanderMaxTimeMs(_spawnInfo.wanderMaxTimeMs),
	m_spawnPosition(_spawnInfo.spawnPosition),
	m_destination(_spawnInfo.spawnPosition),
	m_wanderRadius(_spawnInfo.wanderRadius),
	m_maxAggroRange(_spawnInfo.maxAggroRange),
	m_respawnRadius(_spawnInfo.respawnRadius),
	m_respawnDelayMs(_spawnInfo.respawnDelayMs),
	m_nextAttackTime(0),
	m_respawnTime(0),
	m_nextWanderTime(0),
	m_wanderEndTime(0),
	m_lastUpdateTime(0),
	m_targetCellIndex(UINT64_MAX),
	m_pathIndex(0),
	m_pathVersion(0),
	m_sentPathVersion(0),
	m_attackTargetObjectNum(UINT64_MAX),
	m_lastAttackDamage(0)
{
	if (m_level == 0) m_level = 1;
	if (m_maxHP == 0) m_maxHP = 1;
	if (m_attackPower == 0) m_attackPower = CCombatDamageCalculator::CalculateMonsterDamage(m_level);
	m_hp = m_maxHP;
	m_spawnPosition.y = 0.0f;
	m_position = m_spawnPosition;
	m_rotationY = _spawnInfo.rotationY;
	m_nextWanderTime = GetTickCount64() + GetRandomTime(m_idleMinTimeMs, m_idleMaxTimeMs, 0);
}

void CMonster::Update(uint64_t _currentTime, uint64_t _deltaTimeMs, CPlayerManager* _playerManager)
{
	if (m_lastUpdateTime != 0) _deltaTimeMs = _currentTime - m_lastUpdateTime;
	m_lastUpdateTime = _currentTime;
	m_lastAttackDamage = 0;
	if (m_state == eMonsterState::Dead)
	{
		UpdateDead(_currentTime);
		return;
	}
	if (m_state == eMonsterState::Return)
	{
		UpdateReturn(_deltaTimeMs);
		return;
	}
	if (VECTOR3::Distance(m_position, m_spawnPosition) > m_maxAggroRange)
	{
		StartReturn();
		return;
	}
	CPlayer* target = nullptr;
	if (_playerManager != nullptr) target = _playerManager->Find(m_attackTargetObjectNum);
	if (m_attackTargetObjectNum != UINT64_MAX && (target == nullptr || target->GetField() != m_field || target->GetState() == ePlayerState::Die))
	{
		StartReturn();
		return;
	}
	if (m_state == eMonsterState::Idle || m_state == eMonsterState::Move)
	{
		if (m_aggressive && target == nullptr)
		{
			target = FindNearbyPlayer();
			if (target != nullptr) m_attackTargetObjectNum = target->GetNum();
		}
		UpdateIdle(_currentTime, _deltaTimeMs, target);
	}
	if (m_state == eMonsterState::Chase) UpdateChase(_currentTime, _deltaTimeMs, target);
	if (m_state == eMonsterState::Attack) UpdateAttack(_currentTime, target);
}

uint64_t CMonster::TakeDamage(uint64_t _damage)
{
	return TakeDamage(nullptr, _damage, GetTickCount64());
}

uint64_t CMonster::TakeDamage(CPlayer* _attacker, uint64_t _damage, uint64_t _currentTime)
{
	if (m_invincible || m_state == eMonsterState::Dead || m_state == eMonsterState::Return) return 0;
	if (_attacker != nullptr && (!_attacker->IsActive() || _attacker->GetField() != m_field || _attacker->GetState() == ePlayerState::Die)) return 0;
	if (_damage >= m_hp)
		m_hp = 0;
	else
		m_hp -= _damage;
	if (m_hp == 0)
	{
		if (_attacker != nullptr) _attacker->AddExperience(m_experience);
		Die(_currentTime);
		return m_experience;
	}
	if (_attacker == nullptr) return 0;
	// 같은 타겟이면 기존 공격과 경로 유지
	if (m_attackTargetObjectNum != _attacker->GetNum() || m_state == eMonsterState::Idle || m_state == eMonsterState::Move)
	{
		m_attackTargetObjectNum = _attacker->GetNum();
		ClearPath();
		m_state = eMonsterState::Chase;
	}
	return 0;
}

void CMonster::ReleaseTarget(uint64_t _playerObjectNum)
{
	if (m_attackTargetObjectNum != _playerObjectNum) return;
	m_attackTargetObjectNum = UINT64_MAX;
	if (m_state == eMonsterState::Chase || m_state == eMonsterState::Attack) StartReturn();
}

CPlayer* CMonster::FindNearbyPlayer()
{
	if (m_sector == nullptr) return nullptr;
	std::vector<CSector*> sectorList;
	sectorList.push_back(m_sector);
	std::vector<CSector*>& adjoining = m_sector->GetAdjoiningSectorList();
	uint64_t adjoiningSize = static_cast<uint64_t>(adjoining.size());
	for (uint64_t i = 0; i < adjoiningSize; ++i)
	{
		if (adjoining[i] != nullptr) sectorList.push_back(adjoining[i]);
	}
	CPlayer* result = nullptr;
	float resultDistance = m_detectionRange;
	uint64_t sectorSize = static_cast<uint64_t>(sectorList.size());
	for (uint64_t i = 0; i < sectorSize; ++i)
	{
		std::set<CFieldObject*>& objectList = sectorList[i]->GetObjectList();
		for (std::set<CFieldObject*>::iterator iter = objectList.begin(); iter != objectList.end(); ++iter)
		{
			CFieldObject* object = *iter;
			if (object->GetObjectType() != eFieldObjectType::Player || !object->IsActive()) continue;
			CPlayer* player = static_cast<CPlayer*>(object);
			if (player->GetState() == ePlayerState::Die) continue;
			float distance = VECTOR3::Distance(m_position, player->GetPosition());
			if (distance > resultDistance) continue;
			result = player;
			resultDistance = distance;
		}
	}
	return result;
}

void CMonster::UpdateIdle(uint64_t _currentTime, uint64_t _deltaTimeMs, CPlayer* _target)
{
	if (_target != nullptr)
	{
		if (m_canAttack && VECTOR3::Distance(m_position, _target->GetPosition()) <= m_attackRange)
		{
			ClearPath();
			m_state = eMonsterState::Attack;
			return;
		}
		if (m_canChase)
		{
			ClearPath();
			m_state = eMonsterState::Chase;
			return;
		}
		m_attackTargetObjectNum = UINT64_MAX;
	}
	if (m_state == eMonsterState::Move)
	{
		// 배회 종료 시각까지만 이동
		uint64_t moveTime = _deltaTimeMs;
		uint64_t previousTime = _currentTime - _deltaTimeMs;
		if (previousTime >= m_wanderEndTime)
			moveTime = 0;
		else if (_currentTime > m_wanderEndTime)
			moveTime = m_wanderEndTime - previousTime;
		bool moving = MovePath(moveTime);
		if (_currentTime >= m_wanderEndTime || !moving)
		{
			ClearPath();
			m_state = eMonsterState::Idle;
			m_nextWanderTime = _currentTime + GetRandomTime(m_idleMinTimeMs, m_idleMaxTimeMs, _currentTime);
		}
		return;
	}
	if (!m_canWander || _currentTime < m_nextWanderTime || m_wanderRadius <= 0.0f) return;
	// 스폰 반경 내에서 선택. Block이면 이동x
	m_nextWanderTime = _currentTime + GetRandomTime(m_idleMinTimeMs, m_idleMaxTimeMs, _currentTime);
	float angle = CRandomNumberGenerator::Genarate_float(0.0f, 6.2831853f);
	float radius = m_wanderRadius * std::sqrt(CRandomNumberGenerator::Genarate_float(0.25f, 1.0f));
	m_destination.x = m_spawnPosition.x + std::sin(angle) * radius;
	m_destination.z = m_spawnPosition.z + std::cos(angle) * radius;
	m_destination.y = 0.0f;
	if (m_field == nullptr || !m_field->IsWalkable(m_destination)) return;
	if (VECTOR3::Distance(m_position, m_destination) < 1.5f) return;
	if (!CreatePath(m_destination))
	{
		return;
	}
	m_wanderEndTime = _currentTime + GetRandomTime(m_wanderMinTimeMs, m_wanderMaxTimeMs, _currentTime);
	m_state = eMonsterState::Move;
}

void CMonster::UpdateChase(uint64_t _currentTime, uint64_t _deltaTimeMs, CPlayer* _target)
{
	if (_target == nullptr || !m_canChase)
	{
		ClearPath();
		m_state = eMonsterState::Idle;
		return;
	}
	if (VECTOR3::Distance(_target->GetPosition(), m_spawnPosition) > m_maxAggroRange)
	{
		StartReturn();
		return;
	}
	bool clearLine = m_field->IsClearLine(m_position, _target->GetPosition());
	if (clearLine && VECTOR3::Distance(m_position, _target->GetPosition()) <= m_attackRange)
	{
		ClearPath();
		m_state = eMonsterState::Attack;
		UpdateAttack(_currentTime, _target);
		return;
	}
	CCell* targetCell = m_field->GetCellGrid()->GetCell(_target->GetPosition());
	if (targetCell == nullptr)
	{
		StartReturn();
		return;
	}
	// 타겟 사거리까지만 이동
	float remaining = VECTOR3::Distance(m_position, _target->GetPosition()) - m_attackRange * 0.9f;
	if (clearLine && m_moveSpeed > 0.0f && remaining > 0.0f)
	{
		uint64_t approachTime = static_cast<uint64_t>(remaining / m_moveSpeed * 1000.0f);
		if (_deltaTimeMs > approachTime) _deltaTimeMs = approachTime;
	}
	// 기존 경로로 이동한 뒤 경로 갱신
	if (!m_path.empty() && !MovePath(_deltaTimeMs)) ClearPath();
	if (m_field->IsClearLine(m_position, _target->GetPosition()) && VECTOR3::Distance(m_position, _target->GetPosition()) <= m_attackRange)
	{
		ClearPath();
		m_state = eMonsterState::Attack;
		return;
	}
	if (m_path.empty() || m_targetCellIndex != targetCell->GetIndex())
	{
		if (!CreateChasePath(_target->GetPosition()))
		{
			StartReturn();
			return;
		}
		m_targetCellIndex = targetCell->GetIndex();
	}
}

void CMonster::UpdateAttack(uint64_t _currentTime, CPlayer* _target)
{
	if (_target == nullptr || !_target->IsActive())
	{
		m_attackTargetObjectNum = UINT64_MAX;
		m_state = eMonsterState::Idle;
		return;
	}
	if (VECTOR3::Distance(_target->GetPosition(), m_spawnPosition) > m_maxAggroRange)
	{
		StartReturn();
		return;
	}
	if (VECTOR3::Distance(m_position, _target->GetPosition()) > m_attackRange || !m_field->IsClearLine(m_position, _target->GetPosition()))
	{
		if (m_canChase)
			m_state = eMonsterState::Chase;
		else
		{
			m_attackTargetObjectNum = UINT64_MAX;
			m_state = eMonsterState::Idle;
		}
		return;
	}
	if (!m_canAttack)
	{
		m_attackTargetObjectNum = UINT64_MAX;
		m_state = eMonsterState::Idle;
		return;
	}
	VECTOR3 direction = _target->GetPosition() - m_position;
	if (direction.x != 0.0f || direction.z != 0.0f) m_rotationY = std::atan2(direction.x, direction.z) * 57.2957795f;
	if (_currentTime < m_nextAttackTime) return;
	m_nextAttackTime = _currentTime + m_attackIntervalMs;
	m_lastAttackDamage = _target->TakeDamage(m_attackPower);
}

void CMonster::UpdateReturn(uint64_t _deltaTimeMs)
{
	if (VECTOR3::Distance(m_position, m_spawnPosition) <= 0.1f)
	{
		m_position = m_spawnPosition;
		m_destination = m_spawnPosition;
		m_hp = m_maxHP;
		m_invincible = false;
		m_state = eMonsterState::Idle;
		return;
	}
	if (m_path.empty())
	{
		CreatePath(m_spawnPosition);
		return;
	}
	MovePath(_deltaTimeMs);
}

void CMonster::UpdateDead(uint64_t _currentTime)
{
	if (_currentTime < m_respawnTime) return;
	if (m_field == nullptr || !m_field->Move(this, m_spawnPosition)) return;
	m_destination = m_spawnPosition;
	m_hp = m_maxHP;
	ClearPath();
	m_invincible = false;
	m_state = eMonsterState::Idle;
}

bool CMonster::CreatePath(const VECTOR3& _destination)
{
	if (m_field == nullptr || !m_field->IsWalkable(_destination)) return false;
	if (m_field->IsClearLine(m_position, _destination))
	{
		m_path = {_destination};
		m_pathIndex = 0;
		m_destination = _destination;
		++m_pathVersion;
		return true;
	}
	CCellGrid* cellGrid = m_field->GetCellGrid();
	CCell* startCell = cellGrid->GetCell(m_position);
	CCell* arrivalCell = cellGrid->GetCell(_destination);
	if (startCell == nullptr || arrivalCell == nullptr) return false;
	// 인접 셀이 모두 비어 있으면 바로 연결
	uint64_t minX = startCell->GetX(), maxX = arrivalCell->GetX();
	uint64_t minZ = startCell->GetZ(), maxZ = arrivalCell->GetZ();
	if (minX > maxX)
	{
		minX = arrivalCell->GetX();
		maxX = startCell->GetX();
	}
	if (minZ > maxZ)
	{
		minZ = arrivalCell->GetZ();
		maxZ = startCell->GetZ();
	}
	if (maxX - minX <= 1 && maxZ - minZ <= 1)
	{
		bool clear = true;
		for (uint64_t x = minX; x <= maxX; ++x)
		{
			for (uint64_t z = minZ; z <= maxZ; ++z)
			{
				if (cellGrid->GetCellList()[x * cellGrid->GetCols() + z].IsBlock()) clear = false;
			}
		}
		if (clear)
		{
			m_path = {_destination};
			m_pathIndex = 0;
			m_destination = _destination;
			++m_pathVersion;
			return true;
		}
	}
	m_path = m_navigator.Navigate(*cellGrid, startCell, arrivalCell);
	// 셀 중앙 좌표로 변경
	for (uint64_t i = 0; i < m_path.size(); ++i)
	{
		m_path[i].x += 0.5f;
		m_path[i].z += 0.5f;
	}
	if (!m_path.empty())
	{
		VECTOR3 startCenter = startCell->GetPosition();
		startCenter.x += 0.5f;
		startCenter.z += 0.5f;
		// 바로 갈 수 없을 때만 시작 셀 중앙 추가
		if (!m_field->IsClearLine(m_position, m_path[0])) m_path.insert(m_path.begin(), startCenter);
	}
	if (m_path.empty())
	{
		if (startCell != arrivalCell) return false;
		m_path.push_back(_destination);
	}
	else if (!(m_path.back() == _destination))
	{
		// 마지막 꼭짓점은 실제 목적지로 변경
		VECTOR3 previous = m_position;
		if (m_path.size() > 1) previous = m_path[m_path.size() - 2];
		VECTOR3 direction = _destination - previous;
		uint64_t count = static_cast<uint64_t>(std::ceil(direction.Magnitude() * 4.0f)) + 1;
		bool walkable = true;
		for (uint64_t i = 0; i <= count; ++i)
		{
			VECTOR3 position = previous + direction * (static_cast<float>(i) / static_cast<float>(count));
			if (!m_field->IsWalkable(position))
			{
				walkable = false;
				break;
			}
		}
		if (walkable)
			m_path.back() = _destination;
		else
			m_path.push_back(_destination);
	}
	m_pathIndex = 0;
	m_destination = _destination;
	VECTOR3 direction = m_path[0] - m_position;
	if (direction.x != 0.0f || direction.z != 0.0f) m_rotationY = std::atan2(direction.x, direction.z) * 57.2957795f;
	++m_pathVersion;
	return true;
}

bool CMonster::CreateChasePath(const VECTOR3& _targetPosition)
{
	if (!CreatePath(_targetPosition)) return false;
	// 마지막 선분을 줄여 사거리에서 정지
	VECTOR3 previous = m_position;
	if (m_path.size() > 1) previous = m_path[m_path.size() - 2];
	VECTOR3 direction = previous - _targetPosition;
	float distance = direction.Magnitude();
	float stopDistance = m_attackRange * 0.9f;
	if (stopDistance > distance) stopDistance = distance;
	if (distance > 0.0001f && stopDistance > 0.0f)
	{
		VECTOR3 destination = _targetPosition + direction.Normalize() * stopDistance;
		if (m_field->IsClearLine(previous, destination) && m_field->IsClearLine(destination, _targetPosition))
		{
			m_path.back() = destination;
			m_destination = destination;
		}
	}
	return true;
}

bool CMonster::MovePath(uint64_t _deltaTimeMs)
{
	float moveDistance = m_moveSpeed * static_cast<float>(_deltaTimeMs) / 1000.0f;
	while (moveDistance > 0.0f && m_pathIndex < m_path.size())
	{
		VECTOR3 direction = m_path[m_pathIndex] - m_position;
		direction.y = 0.0f;
		float distance = direction.Magnitude();
		// 한 틱 안에 꼭짓점까지 도착해도 이동 방향은 갱신
		if (distance > 0.0001f) m_rotationY = std::atan2(direction.x, direction.z) * 57.2957795f;
		if (distance <= moveDistance)
		{
			if (!m_field->Move(this, m_path[m_pathIndex])) return false;
			moveDistance -= distance;
			++m_pathIndex;
			continue;
		}
		VECTOR3 normalized = direction.Normalize();
		VECTOR3 position = m_position + normalized * moveDistance;
		position.y = 0.0f;
		if (!m_field->Move(this, position)) return false;
		moveDistance = 0.0f;
	}
	return m_pathIndex < m_path.size();
}

uint64_t CMonster::GetRandomTime(uint64_t _minTime, uint64_t _maxTime, uint64_t _currentTime)
{
	if (_maxTime <= _minTime) return _minTime;
	return static_cast<uint64_t>(CRandomNumberGenerator::Genarate_int64(static_cast<int64_t>(_minTime), static_cast<int64_t>(_maxTime)));
}

void CMonster::ClearPath()
{
	if (m_path.empty()) return;
	m_path.clear();
	m_pathIndex = 0;
	m_targetCellIndex = UINT64_MAX;
	++m_pathVersion;
}

void CMonster::StartReturn()
{
	m_attackTargetObjectNum = UINT64_MAX;
	ClearPath();
	m_invincible = true;
	m_state = eMonsterState::Return;
	m_destination = m_spawnPosition;
}

void CMonster::Die(uint64_t _currentTime)
{
	m_attackTargetObjectNum = UINT64_MAX;
	ClearPath();
	m_state = eMonsterState::Dead;
	m_invincible = true;
	m_respawnTime = _currentTime + m_respawnDelayMs;
}
eMonsterType CMonster::GetType()
{
	return m_type;
}
eMonsterState CMonster::GetState()
{
	return m_state;
}
uint64_t CMonster::GetLevel()
{
	return m_level;
}
uint64_t CMonster::GetMaxHP()
{
	return m_maxHP;
}
uint64_t CMonster::GetHP()
{
	return m_hp;
}
uint64_t CMonster::GetExperience()
{
	return m_experience;
}
uint64_t CMonster::GetAttackPower()
{
	return m_attackPower;
}
bool CMonster::IsAggressive()
{
	return m_aggressive;
}
bool CMonster::IsInvincible()
{
	return m_invincible;
}
uint64_t CMonster::GetAttackTargetObjectNum()
{
	return m_attackTargetObjectNum;
}
uint64_t CMonster::GetLastAttackDamage()
{
	return m_lastAttackDamage;
}
uint64_t CMonster::ConsumeAttackDamage()
{
	uint64_t damage = m_lastAttackDamage;
	m_lastAttackDamage = 0;
	return damage;
}

bool CMonster::ConsumePathChanged()
{
	if (m_sentPathVersion == m_pathVersion) return false;
	m_sentPathVersion = m_pathVersion;
	return true;
}
VECTOR3& CMonster::GetSpawnPosition()
{
	return m_spawnPosition;
}
VECTOR3& CMonster::GetDestination()
{
	return m_destination;
}
float CMonster::GetWanderRadius()
{
	return m_wanderRadius;
}
float CMonster::GetMaxAggroRange()
{
	return m_maxAggroRange;
}
float CMonster::GetRespawnRadius()
{
	return m_respawnRadius;
}
uint64_t CMonster::GetRespawnDelayMs()
{
	return m_respawnDelayMs;
}
float CMonster::GetMoveSpeed()
{
	return m_moveSpeed;
}
uint64_t CMonster::GetPathIndex()
{
	return m_pathIndex;
}
uint64_t CMonster::GetPathVersion()
{
	return m_pathVersion;
}
std::vector<VECTOR3>& CMonster::GetPath()
{
	return m_path;
}
