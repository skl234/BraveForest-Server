#pragma once
#include <WinSock2.h>
#include <memory>
#include "ClientConnectionManager.h"
#include "ServerConnectionManager.h"
#include "PacketTask.h"
#include "../Utility_Core/MPSCExecutorManager.h"
#include "../Utility_Core/JobPool.h"
#include "../Network_Core/ServerService.h"
#include "../Network_Core/TCPAcceptor.h"

class CProxyService : public CServerService
{
private:
	CTCPAcceptor			 m_acceptor;
	CClientConnectionManager m_clientConnectionManager;
	CServerConnectionManager m_serverConnectionManager;

	CJobPool				 m_packetTaskPool;
	CMPSCExecutorManager	 m_executorManager;

public:
	CProxyService() = default;
	~CProxyService() = default;

protected:
	bool OnInit() override;
	bool OnRun() override;
	void OnStop() override;
};
