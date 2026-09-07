#pragma once
#include <WinSock2.h>
#include <memory>
#include "../Network_Core/ServerService.h"
#include "../Network_Core/TCPAcceptor.h"
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/JobPool.h"
#include "../Common/ZoneData.h"
#include "../Utility_Core/MPSCExecutorManager.h"

class CLoginService : public CServerService
{
private:
	CTCPAcceptor					m_acceptor;
	std::unique_ptr<CTCPConnection>	m_proxyConnection;
	CJobPool						m_packetTaskPool;
	CZoneData					m_zoneData;
	CMPSCExecutorManager			m_executorManager;

public:
	CLoginService() = default;
	~CLoginService() override = default;

protected:
	bool OnInit() override;
	bool OnRun() override;
	void OnStop() override;
};
