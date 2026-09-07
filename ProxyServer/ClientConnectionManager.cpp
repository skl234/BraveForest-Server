#include "ClientConnectionManager.h"
#include "../Utility_Core/LockGuard.h"

constexpr uint64_t RECV_BUFFER_SIZE = 1024 * 4;

CClientConnectionManager::CClientConnectionManager()
{
	InitializeSRWLock(&m_lock);
}

void CClientConnectionManager::Initialize(uint64_t _size, COverlappedManager* _overlManager)
{
	for (uint64_t i = 0; i < _size; ++i)
	{
		std::unique_ptr<CClientConnection> connection = std::make_unique<CClientConnection>(i, _overlManager, RECV_BUFFER_SIZE);
		m_list.emplace_back(std::move(connection));
		m_pool.emplace_back(m_list.back().get());
	}
}

CClientConnection* CClientConnectionManager::Acquire()
{
	CLockGuard lockGuard(m_lock);
	if (m_pool.empty()) return nullptr;

	CClientConnection* connection = m_pool.front();
	m_pool.pop_front();

	return connection;
}

void CClientConnectionManager::Release(CClientConnection* _connection)
{
	CLockGuard lockGuard(m_lock);
	m_pool.push_back(_connection);
}

CClientConnection* CClientConnectionManager::Find(uint64_t _sessionId)
{
	if (_sessionId >= m_list.size()) return nullptr;

	return m_list[_sessionId].get();
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
