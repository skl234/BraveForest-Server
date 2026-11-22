#include "ClientConnectionManager.h"
#include "../Utility_Core/LockGuard.h"

constexpr uint64_t RECV_BUFFER_SIZE = 1024 * 4;

CClientConnectionManager::CClientConnectionManager()
{
	InitializeSRWLock(&m_lock);
}

void CClientConnectionManager::Initialize(uint64_t _size, CIOCPEventManager* _eventManager)
{
	CLockGuard lockGuard(m_lock);
	if (!m_list.empty())
		return;

	m_list.reserve(_size);
	m_sessionMap.reserve(_size);

	for (uint64_t i = 0; i < _size; ++i)
	{
		std::unique_ptr<CClientConnection> connection = std::make_unique<CClientConnection>(_eventManager, RECV_BUFFER_SIZE);
		m_list.emplace_back(std::move(connection));
		m_pool.emplace_back(m_list.back().get());
	}
}

CClientConnection* CClientConnectionManager::Acquire()
{
	CLockGuard lockGuard(m_lock);
	if (m_pool.empty())
		return nullptr;

	CClientConnection* connection = m_pool.front();
	m_pool.pop_front();

	uint64_t sessionId = ++m_nextSessionId;
	connection->SetSessionId(sessionId);
	connection->SetAcquired(true);
	connection->SetZoneId(0);
	connection->SetAccountIndex(0);
	connection->SetLogoutPending(false);
	connection->SetLoginPending(false);
	m_sessionMap.emplace(sessionId, connection);

	return connection;
}

void CClientConnectionManager::Release(CClientConnection* _connection)
{
	if (_connection == nullptr)
		return;

	CLockGuard lockGuard(m_lock);
	uint64_t sessionId = _connection->GetSessionId();
	std::unordered_map<uint64_t, CClientConnection*>::iterator iter = m_sessionMap.find(sessionId);
	if (iter == m_sessionMap.end() || iter->second != _connection)
		return;
	if (!_connection->SetAcquired(false))
		return;

	m_sessionMap.erase(iter);
	_connection->SetSessionId(0);
	m_pool.push_back(_connection);
}

CClientConnection* CClientConnectionManager::Find(uint64_t _sessionId)
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	std::unordered_map<uint64_t, CClientConnection*>::iterator iter = m_sessionMap.find(_sessionId);
	if (iter == m_sessionMap.end())
		return nullptr;

	return iter->second;
}

std::vector<std::unique_ptr<CClientConnection>>& CClientConnectionManager::GetList()
{
	return m_list;
}

uint64_t CClientConnectionManager::GetAcquireSize()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_list.size() - m_pool.size();
}

uint64_t CClientConnectionManager::GetSize()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_list.size();
}
