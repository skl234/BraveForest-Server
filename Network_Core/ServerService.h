#pragma once
#include <WinSock2.h>
#include "IOCPServer.h"
#include "INI_CONFIG.h"
#include "../Utility_Core/UtilityMacros.h"

class CServerService
{
protected:
	CIOCPServer	m_server;
	INI_CONFIG* m_config;

public:
	CServerService() = default;
	virtual ~CServerService() = default;

	bool Initialize(INI_CONFIG* _config);
	bool Run();
	void Stop();

protected:
	virtual bool OnInit() = 0;
	virtual bool OnRun() = 0;
	virtual void OnStop() = 0;

public:
	DELETE_COPY_MOVE(CServerService);
};
