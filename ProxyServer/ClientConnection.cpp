#include "ClientConnection.h"

CClientConnection::CClientConnection(CIOCPEventManager* _eventManager, uint64_t _recvBufferSize):
	CTCPConnection(_eventManager, _recvBufferSize)
{
}

CClientConnection::~CClientConnection()
{
	Disconnect();
}

void CClientConnection::SetSessionId(uint64_t _sessionId)
{
	m_sessionId.store(_sessionId);
}

uint64_t CClientConnection::GetSessionId()
{
	return m_sessionId.load();
}

void CClientConnection::SetAccountIndex(uint64_t _index)
{
	m_accountIndex.store(_index);
}

uint64_t CClientConnection::GetAccountIndex()
{
	return m_accountIndex.load();
}

void CClientConnection::SetZoneId(uint64_t _zoneId)
{
	m_zoneId.store(_zoneId);
}

uint64_t CClientConnection::GetZoneId()
{
	return m_zoneId.load();
}

bool CClientConnection::TryStartLogout()
{
	bool expected = false;
	return m_logoutPending.compare_exchange_strong(expected, true);
}

void CClientConnection::SetLogoutPending(bool _pending)
{
	m_logoutPending.store(_pending);
}

bool CClientConnection::TryStartLogin()
{
	bool expected = false;
	return m_loginPending.compare_exchange_strong(expected, true);
}

bool CClientConnection::IsLoginPending()
{
	return m_loginPending.load();
}

void CClientConnection::SetLoginPending(bool _pending)
{
	m_loginPending.store(_pending);
}

bool CClientConnection::SetAcquired(bool _acquired)
{
	return m_acquired.exchange(_acquired);
}
