#include "Session.h"
#include "../Utility_Core/LockGuard.h"

CSession::CSession()
{
	InitializeSRWLock(&m_lock);
}

void CSession::Activate(uint64_t _sessionId, uint64_t _accountIndex)
{
	CLockGuard lockGuard(m_lock);
	m_sessionId = _sessionId;
	m_accountIndex = _accountIndex;
	m_active = true;
}

void CSession::SetPlayer(uint64_t _characterIndex, uint64_t _channelId, CPlayer* _player)
{
	CLockGuard lockGuard(m_lock);
	if (!m_active) return;
	if (_characterIndex == 0 || _channelId == 0 || _player == nullptr) return;

	m_characterIndex = _characterIndex;
	m_channelId = _channelId;
	m_player = _player;
}

void CSession::ClearPlayer()
{
	CLockGuard lockGuard(m_lock);
	m_characterIndex = 0;
	m_channelId = 0;
	m_player = nullptr;
}

void CSession::Cleanup()
{
	CLockGuard lockGuard(m_lock);
	m_sessionId = 0;
	m_accountIndex = 0;
	m_characterIndex = 0;
	m_channelId = 0;
	m_player = nullptr;
	m_active = false;
}

uint64_t CSession::GetSessionId()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_sessionId;
}

uint64_t CSession::GetAccountIndex()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_accountIndex;
}

uint64_t CSession::GetCharacterIndex()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_characterIndex;
}

uint64_t CSession::GetChannelId()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_channelId;
}

CPlayer* CSession::GetPlayer()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_player;
}

bool CSession::IsActive()
{
	CLockGuard lockGuard(m_lock, eLockMode::Shared);
	return m_active;
}
