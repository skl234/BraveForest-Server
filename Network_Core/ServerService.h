#pragma once
#include <WinSock2.h>
#include "IOCP.h"
#include "IOCPEventManager.h"
#include "IOCPWorkerManager.h"
#include "INI_CONFIG.h"
#include "../Utility_Core/UtilityMacros.h"

class CServerService
{
public:
	DELETE_COPY_MOVE(CServerService);

protected:
	CIOCP						m_iocp;
	CIOCPEventManager			m_eventManager;
	CIOCPWorkerManager			m_workerManager;
	INI_CONFIG*					m_config = nullptr;

public:
	CServerService() = default;
	virtual ~CServerService() = default;

	bool Initialize(INI_CONFIG* _config);
	bool Run();
	bool Stop();

protected:
	virtual bool OnInit() = 0;
	virtual bool OnRun() = 0;
	virtual void OnStop() = 0;

private:
	void StartWorker();
	void ExitInitialize(const char* _section, const int _error);
};
