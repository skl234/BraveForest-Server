#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <atomic>
#include <vector>
#include <map>
#include "ClientConnectionManager.h"
#include "ServerConnectionManager.h"
#include "../Utility_Core/MPSCExecutorManager.h"
#include "PacketTask.h"
#include "../Network_Core/IOCPServer.h"
#include "../Utility_Core/Singleton.h"
#include "../Utility_Core/JobPool.h"

class CMonitor : public CSingleton<CMonitor>
{
	friend class CSingleton<CMonitor>;

private:
	CClientConnectionManager* m_clientConnManager = nullptr;
	CServerConnectionManager* m_servConnManager = nullptr;
	CIOCPServer*			  m_server = nullptr;			
	CMPSCExecutorManager*	  m_mpscManager = nullptr;
	CJobPool*    m_jobPoolManager = nullptr;

private:
	CMonitor() = default;
	~CMonitor() = default;

public:
	void Initialize(CClientConnectionManager* _clientConnManager, CServerConnectionManager* _servConnManager, 
		CIOCPServer* _server, CMPSCExecutorManager* _mpscManager,
		CJobPool* _jobPool);
	std::vector<byte>	GetMonitorPacket();
};
