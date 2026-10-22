#include "SessionManager.h"
#include <utility>

void CSessionManager::Initialize(uint64_t _size)
{
	m_nameMap.clear();
	m_nameMap.reserve(_size);
	m_sessionList.clear();
	m_sessionList.reserve(_size);
	for (uint64_t i = 0; i < _size; ++i)
	{
		m_sessionList.emplace_back(i);
	}
}

void CSessionManager::Cleanup() noexcept
{
	m_nameMap.clear();
	const uint64_t size = static_cast<uint64_t>(m_sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		m_sessionList[i].Cleanup();
	}
}


bool CSessionManager::Register(uint64_t _sessionId, uint64_t _accountIndex) noexcept
{
	if (_sessionId >= m_sessionList.size() || _accountIndex == 0) return false;

	CSession* session = &m_sessionList[_sessionId];

	// 이미 사용 중인 세션이면 등록 불가
	if (session->IsActive()) return false;

	session->Initialize(_accountIndex);
	return true;
}

bool CSessionManager::Unregister(uint64_t _sessionId, uint64_t _accountIndex) noexcept
{
	if (_sessionId >= m_sessionList.size() || _accountIndex == 0) return false;

	CSession* session = Find(_sessionId);
	if (session == nullptr) return true;
	if (session->GetAccountIndex() != _accountIndex) return false;

	UnregisterPlayer(_sessionId);
	session->Cleanup();
	return true;
}


bool CSessionManager::RegisterPlayer(uint64_t _sessionId, const PLAYER_DATA& _data, const std::string& _name)
{
	CSession* session = Find(_sessionId);
	if (session == nullptr || session->IsClosing() || session->GetAccountIndex() != _data.accountIndex) return false;
	if (session->GetPlayer().GetCharacterIndex() != 0 || _data.characterIndex == 0) return false;
	if (_name.empty() || _name.size() > 30 || _name.find('\0') != std::string::npos) return false;

	std::string name = _name;
	if (!m_nameMap.emplace(_name, _sessionId).second) return false;

	// 이름 등록까지 끝난 뒤 Player 교체
	session->GetPlayer().Initialize(_data.characterIndex, std::move(name), _data.level, _data.jobClass);
	return true;
}

void CSessionManager::UnregisterPlayer(uint64_t _sessionId) noexcept
{
	CSession* session = Find(_sessionId);
	if (session == nullptr) return;

	std::unordered_map<std::string, uint64_t>::iterator iter = m_nameMap.find(session->GetPlayer().GetName());
	if (iter != m_nameMap.end() && iter->second == _sessionId) m_nameMap.erase(iter);
	session->GetPlayer().Cleanup();
}


CSession* CSessionManager::Find(uint64_t _sessionId) noexcept
{
	if (_sessionId >= m_sessionList.size()) return nullptr;
	if (!m_sessionList[_sessionId].IsActive()) return nullptr;
	return &m_sessionList[_sessionId];
}

const CSession* CSessionManager::Find(uint64_t _sessionId) const noexcept
{
	if (_sessionId >= m_sessionList.size()) return nullptr;
	if (!m_sessionList[_sessionId].IsActive()) return nullptr;
	return &m_sessionList[_sessionId];
}

const CSession* CSessionManager::FindByName(const std::string& _name) const
{
	std::unordered_map<std::string, uint64_t>::const_iterator iter = m_nameMap.find(_name);
	if (iter == m_nameMap.end()) return nullptr;

	const CSession* session = Find(iter->second);
	if (session == nullptr || session->GetPlayer().GetName() != _name) return nullptr;
	return session;
}

const std::vector<CSession>& CSessionManager::GetList() const noexcept
{
	return m_sessionList;
}
