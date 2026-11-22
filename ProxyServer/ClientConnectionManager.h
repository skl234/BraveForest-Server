#pragma once
#include <WinSock2.h>
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>
#include "ClientConnection.h"
#include "../Utility_Core/UtilityMacros.h"

class CClientConnectionManager
{
private:
	std::vector<std::unique_ptr<CClientConnection>> m_list;
	std::deque<CClientConnection*>					m_pool;

	std::unordered_map<uint64_t, CClientConnection*> m_sessionMap;
	uint64_t										m_nextSessionId = 0;

	SRWLOCK											m_lock;

public:
	CClientConnectionManager();
	~CClientConnectionManager() = default;

	void			   Initialize(uint64_t _size, CIOCPEventManager* _eventManager);
	CClientConnection* Acquire();
	void			   Release(CClientConnection* _connection);
	CClientConnection* Find(uint64_t _sessionId);

	std::vector<std::unique_ptr<CClientConnection>>& GetList();
	uint64_t GetAcquireSize();
	uint64_t GetSize();
};
