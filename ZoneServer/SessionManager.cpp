#include "SessionManager.h"
#include "../Utility_Core/LockGuard.h"

CSessionManager::CSessionManager()
{
	InitializeSRWLock(&m_lock);
}

void CSessionManager::Initialize(uint64_t _size)
{
	CLockGuard lockGuard(m_lock);
	if (!m_list.empty()) return;

	m_list.reserve(_size);
	m_sessionMap.reserve(_size);

	for (uint64_t i = 0; i < _size; ++i)
	{
		auto session = std::make_unique<CSession>();
		m_list.emplace_back(std::move(session));
		m_pool.emplace_back(m_list.back().get());
	}
}

CSession* CSessionManager::Activate(uint64_t _sessionId, uint64_t _accountIndex)
{
	CLockGuard lockGuard(m_lock);
	if (_accountIndex == 0) return nullptr;
	if (m_sessionMap.find(_sessionId) != m_sessionMap.end()) return nullptr;
	if (m_pool.empty()) return nullptr;

	auto session = m_pool.front();
	m_pool.pop_front();
	session->Activate(_sessionId, _accountIndex);
	m_sessionMap.emplace(_sessionId, session);

	return session;
}

CSession* CSessionManager::Find(uint64_t _sessionId)
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	auto iter = m_sessionMap.find(_sessionId);
	if (iter == m_sessionMap.end()) return nullptr;

	return iter->second;
}

std::vector<CSession*> CSessionManager::GetActiveSessionList()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	std::vector<CSession*> sessionList;
	sessionList.reserve(m_sessionMap.size());
	std::unordered_map<uint64_t, CSession*>::iterator iter = m_sessionMap.begin();
	std::unordered_map<uint64_t, CSession*>::iterator end = m_sessionMap.end();
	for (iter; iter != end; ++iter)
	{
		sessionList.push_back(iter->second);
	}
	return sessionList;
}

void CSessionManager::Deactivate(uint64_t _sessionId)
{
	CLockGuard lockGuard(m_lock);
	auto iter = m_sessionMap.find(_sessionId);
	if (iter == m_sessionMap.end()) return;

	auto session = iter->second;
	session->Cleanup();

	m_sessionMap.erase(iter);
	m_pool.emplace_back(session);
}
