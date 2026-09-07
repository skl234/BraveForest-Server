#include "ClientConnection.h"
#include "ClientConnectionManager.h"
#include "../Common/PACKET_HEADER.h"
#include "../Utility_Core/LogManager.h"
#include "Monitor.h"
#include <vector>

CClientConnection::CClientConnection(const uint64_t _sessionId, COverlappedManager* _overlManager, uint64_t _recvBufferSize):
	CTCPConnection(_overlManager, _recvBufferSize),
	m_sessionId(_sessionId),
	m_accountIndex(NULL)
{
}

CClientConnection::~CClientConnection()
{
	Disconnect();
}

void CClientConnection::SendMonitorPacket()
{
	std::vector<byte> packet = CMonitor::GetInstance().GetMonitorPacket();
	Send(reinterpret_cast<char*>(packet.data()), packet.size());
}

uint64_t CClientConnection::GetSessionId()
{
	return m_sessionId;
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
