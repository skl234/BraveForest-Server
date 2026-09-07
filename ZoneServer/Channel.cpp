#include "Channel.h"

CChannel::CChannel() :
	m_channelId(0),
	m_active(false),
	m_playerCount(0),
	m_reservedPlayerCount(0),
	m_maxPlayerCount(0)
{
}

CChannel::~CChannel()
{
	Stop();
}

bool CChannel::Initialize(uint64_t _channelId, uint64_t _playerPoolSize, const std::string& _fieldInfoPath, const std::vector<MONSTER_SPAWN_INFO>* _spawnInfoList, const CPlayerStatDB* _playerStatDB)
{
	if (_channelId == 0) return false;
	if (_playerPoolSize == 0) return false;

	std::vector<MONSTER_SPAWN_INFO> spawnInfoList;
	if (!m_field.Initialize(_fieldInfoPath, spawnInfoList, _spawnInfoList == nullptr)) return false;
	if (_spawnInfoList != nullptr) spawnInfoList = *_spawnInfoList;
	if (spawnInfoList.size() > 512) return false;
	if (!m_monsterManager.Initialize(spawnInfoList)) return false;
	m_playerManager.Initialize(_playerPoolSize, _playerStatDB);

	std::vector<std::unique_ptr<CMonster>>& monsterList = m_monsterManager.GetList();
	uint64_t monsterSize = static_cast<uint64_t>(monsterList.size());
	for (uint64_t i = 0; i < monsterSize; ++i)
	{
		if (!m_field.Enter(monsterList[i].get())) return false;
	}

	m_channelId = _channelId;
	m_maxPlayerCount = _playerPoolSize;
	if (!m_monsterScheduler.Initialize(this, &m_monsterManager, 500)) return false;
	if (!m_jobExecutor.Initialize()) return false;
	return true;
}

bool CChannel::Start()
{
	if (!m_jobExecutor.Start()) return false;
	m_active.store(true, std::memory_order_relaxed);
	return true;
}

void CChannel::Stop()
{
	m_active.store(false, std::memory_order_relaxed);
	m_jobExecutor.Stop();
}

void CChannel::PushJob(CJob* _job)
{
	if (_job == nullptr) return;
	if (!m_active.load(std::memory_order_relaxed)) return;
	m_jobExecutor.Add(_job);
}

void CChannel::ScheduleMonster(uint64_t _currentTime)
{
	if (!m_active.load(std::memory_order_relaxed)) return;
	if (!m_monsterScheduler.TrySchedule(_currentTime)) return;
	m_jobExecutor.Add(&m_monsterScheduler);
}

bool CChannel::IsActive()
{
	return m_active.load(std::memory_order_relaxed);
}

void CChannel::SetMonsterUpdateHandler(const std::function<void(CChannel*, CMonster*)>& _handler)
{
	m_monsterUpdateHandler = _handler;
}

void CChannel::NotifyMonsterUpdated(CMonster* _monster)
{
	if (_monster == nullptr || !m_monsterUpdateHandler) return;
	m_monsterUpdateHandler(this, _monster);
}

CPlayer* CChannel::CreatePlayer(uint64_t _characterIndex, const std::string& _name, eClassState _class, const VECTOR3& _position, uint64_t _level, uint64_t _experience)
{
	CPlayer* player = m_playerManager.Acquire();
	if (player == nullptr) return nullptr;

	player->Initialize(_characterIndex, _name, _class, _level, _experience);
	player->UpdatePosition(_position);
	// 입장 위치로 목적지도 초기화
	player->UpdateDestPosition(_position);
	if (!m_field.Enter(player))
	{
		m_playerManager.Release(player);
		return nullptr;
	}

	m_playerCount.fetch_add(1, std::memory_order_relaxed);
	return player;
}

CPlayer* CChannel::CreateReservedPlayer(uint64_t _characterIndex, const std::string& _name, eClassState _class, const VECTOR3& _position, uint64_t _level, uint64_t _experience)
{
	uint64_t reservedCount = m_reservedPlayerCount.load(std::memory_order_relaxed);
	if (reservedCount == 0) return nullptr;
	m_reservedPlayerCount.fetch_sub(1, std::memory_order_relaxed);
	return CreatePlayer(_characterIndex, _name, _class, _position, _level, _experience);
}

bool CChannel::ReservePlayer()
{
	while (true)
	{
		uint64_t reservedCount = m_reservedPlayerCount.load(std::memory_order_relaxed);
		if (m_playerCount.load(std::memory_order_relaxed) + reservedCount >= m_maxPlayerCount) return false;
		if (m_reservedPlayerCount.compare_exchange_weak(reservedCount, reservedCount + 1, std::memory_order_relaxed, std::memory_order_relaxed)) return true;
	}
}

void CChannel::CancelPlayerReservation()
{
	uint64_t reservedCount = m_reservedPlayerCount.load(std::memory_order_relaxed);
	if (reservedCount == 0) return;
	m_reservedPlayerCount.fetch_sub(1, std::memory_order_relaxed);
}

void CChannel::RemovePlayer(CPlayer* _player)
{
	if (_player == nullptr) return;
	if (_player->GetField() != &m_field) return;

	m_field.Leave(_player);
	// 풀에 반환하기 전에 타겟 번호 해제
	std::vector<std::unique_ptr<CMonster>>& monsterList = m_monsterManager.GetList();
	uint64_t size = static_cast<uint64_t>(monsterList.size());
	for (uint64_t i = 0; i < size; ++i)
		monsterList[i]->ReleaseTarget(_player->GetNum());
	m_playerManager.Release(_player);
	m_playerCount.fetch_sub(1, std::memory_order_relaxed);
}

bool CChannel::MovePlayer(CPlayer* _player, const VECTOR3& _position)
{
	if (_player == nullptr) return false;
	if (_player->GetState() == ePlayerState::Die) return false;
	if (_player->GetField() != &m_field) return false;
	return m_field.Move(_player, _position);
}

uint64_t CChannel::GetChannelId()
{
	return m_channelId;
}

uint64_t CChannel::GetPlayerCount()
{
	return m_playerCount.load(std::memory_order_relaxed);
}

uint64_t CChannel::GetMaxPlayerCount()
{
	return m_maxPlayerCount;
}

uint64_t CChannel::GetReservedPlayerCount()
{
	return m_reservedPlayerCount.load(std::memory_order_relaxed);
}

CField* CChannel::GetField()
{
	return &m_field;
}

CPlayerManager* CChannel::GetPlayerManager()
{
	return &m_playerManager;
}

CMonsterManager* CChannel::GetMonsterManager()
{
	return &m_monsterManager;
}
