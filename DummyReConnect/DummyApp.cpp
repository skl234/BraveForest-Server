#include "DummyApp.h"
#include "../Network_Core/IOCP.h"
#include "../Network_Core/OverlappedManager.h"
#include "../Utility_Core/JobManager.h"
#include "../Utility_Core/Timer.h"
#include "DummyAppRegistry.h"
#include "DummyMonitor.h"
#include "DummyManager.h"
#include "DummyScheduler.h"
#include <memory>
#include <iostream>
#include <vector>
#include <conio.h>

CDummyApp::CDummyApp()
{
}

CDummyApp::~CDummyApp()
{
}

bool CDummyApp::Initialize(std::string& _ip, uint16_t _port, uint32_t _dummyCount)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { std::cout << "WSAStartup Fail : " << WSAGetLastError() << "\n"; return false; }
	
	if (!CIOCP::GetInstance().Initialize(6)) { std::cout << "IOCP Init Fail: " << WSAGetLastError() << "\n"; return false; }

	CDummyAppRegistry::GetInstance().Initialize(_ip, _port);
	CDummyManager::GetInstance().Initialize(_dummyCount);
	COverlappedManager::GetInstance().Initialize(_dummyCount * 3);

	return true;
}

void CDummyApp::Run(uint16_t _threadCount, int64_t _interval, int64_t _per)
{
	if (!CIOCP::GetInstance().Start()) { std::cout << "IOCP Start Fail: " << WSAGetLastError() << "\n"; return; }

	//uint16_t schedulerSize = CThread::GetNumberOfProcessors();

	std::string ip = CDummyAppRegistry::GetInstance().GetIP();
	uint16_t port = CDummyAppRegistry::GetInstance().GetPort();

	CDummyManager::GetInstance().AllConnect(ip, port);

	std::vector<std::unique_ptr<CDummyScheduler>> schedulerList;
	schedulerList.reserve(_threadCount);
	for (uint16_t i = 0; i < _threadCount; ++i)
	{
		schedulerList.emplace_back(std::make_unique<CDummyScheduler>(_interval, _per));
	}

	for (uint16_t i = 0; i < _threadCount; ++i)
	{
		schedulerList[i]->Start();
	}

	//std::cout << "단위 : 초\n";

	while (true)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'q' || ch == 'Q') break;
		}

		//std::cout << "Connect Count : " << CDummyMonitor::GetInstance().GetConnectCount() << " , ";
		//std::cout << "ConnectFail Count : " << CDummyMonitor::GetInstance().GetConnectFailCount() << "\n";

		CDummyMonitor::GetInstance().ShowMonitor();
		CDummyMonitor::GetInstance().Reset();
		Sleep(1000);
	}
}

void CDummyApp::Stop()
{
}
