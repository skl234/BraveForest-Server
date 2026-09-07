#include "PlayerManager.h"

void CPlayerManager::Initialize(uint64_t _size, const CPlayerStatDB* _playerStatDB)
{
	if (!m_list.empty()) return;

	m_list.reserve(_size);
	m_acquiredList.resize(_size, false);

	for (uint64_t i = 0; i < _size; ++i)
	{
		std::unique_ptr<CPlayer> player = std::make_unique<CPlayer>(i, _playerStatDB);
		m_list.emplace_back(std::move(player));
		m_pool.emplace_back(m_list.back().get());
	}
}

CPlayer* CPlayerManager::Acquire()
{
	if (m_pool.empty()) return nullptr;

	CPlayer* player = m_pool.front();
	m_pool.pop_front();
	m_acquiredList[player->GetNum()] = true;
	return player;
}

void CPlayerManager::Release(CPlayer* _player)
{
	if (_player == nullptr) return;
	if (_player->IsActive()) return;

	uint64_t objectNum = _player->GetNum();
	if (objectNum >= m_list.size()) return;
	if (m_list[objectNum].get() != _player) return;
	if (!m_acquiredList[objectNum]) return;

	_player->Cleanup();
	m_acquiredList[objectNum] = false;
	m_pool.emplace_back(_player);
}

CPlayer* CPlayerManager::Find(uint64_t _objectNum)
{
	if (_objectNum >= m_list.size()) return nullptr;

	CPlayer* player = m_list[_objectNum].get();
	if (!player->IsActive()) return nullptr;
	return player;
}

uint64_t CPlayerManager::GetAcquireSize()
{
	return static_cast<uint64_t>(m_list.size() - m_pool.size());
}

uint64_t CPlayerManager::GetSize()
{
	return static_cast<uint64_t>(m_list.size());
}
