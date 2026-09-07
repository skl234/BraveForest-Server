#pragma once
#include <WinSock2.h>
#include <vector>
#include <memory>
#include "../Network_Core/TCPConnection.h"
#include "../Network_Core/IOCP.h"
#include "../Network_Core/OverlappedManager.h"
#include "../Common/BACK_SERVER_INFO.h"

class CServerConnectionManager
{
private:
	std::unique_ptr<CTCPConnection>				m_loginServerConnection;
	std::vector<std::unique_ptr<CTCPConnection>>	m_zoneServerConnectionList;

public:
	CServerConnectionManager() = default;
	~CServerConnectionManager() = default;

	bool Initialize(CIOCP* _iocp, COverlappedManager* _overlManager, const std::vector<BACK_SERVER_INFO>& _infoList);
	void DisconnectAll();

	CTCPConnection* GetLoginConnection();
	CTCPConnection* GetZoneConnection(uint64_t _zoneId);
	std::vector<std::unique_ptr<CTCPConnection>>& GetZoneConnectionList();
};
