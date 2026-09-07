#pragma once
#include <WinSock2.h>
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>
#include "Session.h"

class CSessionManager
{
private:
	std::vector<std::unique_ptr<CSession>>	m_list;
	std::deque<CSession*>					m_pool;
	std::unordered_map<uint64_t, CSession*>	m_sessionMap;
	SRWLOCK									m_lock;

public:
	CSessionManager();
	~CSessionManager() = default;

	void Initialize(uint64_t _size);
	CSession* Activate(uint64_t _sessionId, uint64_t _accountIndex);
	CSession* Find(uint64_t _sessionId);
	std::vector<CSession*> GetActiveSessionList();
	void Deactivate(uint64_t _sessionId);
};
