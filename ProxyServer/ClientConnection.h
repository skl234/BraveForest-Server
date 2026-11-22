#pragma once
#include <WinSock2.h>
#include "../Network_Core/TCPConnection.h"

class CClientConnection : public CTCPConnection
{
private:
	std::atomic<uint64_t> m_sessionId = 0;
	std::atomic<uint64_t> m_accountIndex = 0;	// 0 : Login X
	std::atomic<uint64_t> m_zoneId = 0;
	std::atomic<bool>	  m_logoutPending = false;
	std::atomic<bool>	  m_loginPending = false;
	std::atomic<bool>	  m_acquired = false;
	
public:
	CClientConnection(CIOCPEventManager* _eventManager, uint64_t _recvBufferSize);
	~CClientConnection() override;

public:
	void	 SetSessionId(uint64_t _sessionId);
	uint64_t GetSessionId();
	void	 SetAccountIndex(uint64_t _index);
	uint64_t GetAccountIndex();
	void	 SetZoneId(uint64_t _zoneId);
	uint64_t GetZoneId();
	bool	 TryStartLogout();
	void	 SetLogoutPending(bool _pending);
	bool	 TryStartLogin();
	bool	 IsLoginPending();
	void	 SetLoginPending(bool _pending);
	bool	 SetAcquired(bool _acquired);
};
