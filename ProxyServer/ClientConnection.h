#pragma once
#include <WinSock2.h>
#include "../Network_Core/TCPConnection.h"

class CClientConnection : public CTCPConnection
{
private:
	uint64_t			  m_sessionId;
	std::atomic<uint64_t> m_accountIndex;	//NULL : Login X
	std::atomic<uint64_t> m_zoneId = NULL;	
	std::atomic<bool>	  m_logoutPending = false;
	
public:
	CClientConnection(const uint64_t _sessionId, COverlappedManager* _overlManager, uint64_t _recvBufferSize);
	~CClientConnection() override;

public:
	void	 SendMonitorPacket();
	uint64_t GetSessionId();
	void	 SetAccountIndex(uint64_t _index);
	uint64_t GetAccountIndex();
	void	 SetZoneId(uint64_t _zoneId);
	uint64_t GetZoneId();
	bool	 TryStartLogout();
	void	 SetLogoutPending(bool _pending);
};
