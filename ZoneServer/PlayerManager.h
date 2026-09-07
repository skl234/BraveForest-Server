#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>
#include "Player.h"

class CPlayerManager
{
private:
	std::vector<std::unique_ptr<CPlayer>>	m_list;
	std::vector<bool>						m_acquiredList;
	std::deque<CPlayer*>					m_pool;

public:
	CPlayerManager() = default;
	~CPlayerManager() = default;

	void Initialize(uint64_t _size, const CPlayerStatDB* _playerStatDB = nullptr);
	CPlayer* Acquire();
	void Release(CPlayer* _player);
	CPlayer* Find(uint64_t _objectNum);

	uint64_t GetAcquireSize();
	uint64_t GetSize();
};
