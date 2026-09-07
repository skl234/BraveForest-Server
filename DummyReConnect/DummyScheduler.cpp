#include "DummyScheduler.h"
#include "DummyManager.h"
#include "DummyAppRegistry.h"
#include "../Utility_Core/Timer.h"
#include "DummyMonitor.h"
#include "DummyAppRegistry.h"
#include "DummyManager.h"
#include "../Utility_Core/Timer.h"
#include "../Utility_Core/RandomNumberGenerator.h"
#include "../Common/PACKET.h"
#include <iostream>

CDummyScheduler::CDummyScheduler(int64_t _interval, int64_t _sendCountPerInterval):
	m_interval(_interval),
	m_sendCountPerInterval(_sendCountPerInterval)
{
}

CDummyScheduler::~CDummyScheduler()
{
	m_run = false;
	WaitForSingleObject(m_handle, INFINITE);
}

void CDummyScheduler::Run()
{
	std::string ip = CDummyAppRegistry::GetInstance().GetIP();
	uint16_t port = CDummyAppRegistry::GetInstance().GetPort();
	CDummyManager& managerInstance = CDummyManager::GetInstance();
	CDummyMonitor& monitorInstance = CDummyMonitor::GetInstance();

	int64_t prevTick = CTimer::GetCurrentTick();

	while (m_run.load())
	{
		while (CTimer::GetCurrentTick() - prevTick < m_interval)
		{
			Sleep(0);
		}
		prevTick = CTimer::GetCurrentTick();

		for (int64_t i = 0; i < m_sendCountPerInterval; ++i)
		{
			CDummyClient* client = managerInstance.Acquire();
			if (!client) continue;

			int num = CRandomNumberGenerator::Genarate_int(1, 4);
			bool result = false;
			if (num == 1)
			{
				PACKET_C2S_DUMMY_PACKET8 packet;
				result = client->Send(reinterpret_cast<char*>(&packet), sizeof(PACKET_C2S_DUMMY_PACKET8));
			}
			if (num == 2)
			{
				PACKET_C2S_DUMMY_PACKET16 packet;
				result = client->Send(reinterpret_cast<char*>(&packet), sizeof(PACKET_C2S_DUMMY_PACKET16));
			}
			if (num == 3)
			{
				PACKET_C2S_DUMMY_PACKET32 packet;
				result = client->Send(reinterpret_cast<char*>(&packet), sizeof(PACKET_C2S_DUMMY_PACKET32));
			}
			if (num == 4)
			{
				PACKET_C2S_DUMMY_PACKET64 packet;
				result = client->Send(reinterpret_cast<char*>(&packet), sizeof(PACKET_C2S_DUMMY_PACKET64));
			}

			if (result) monitorInstance.AddSendCount();
			else monitorInstance.AddSendFailCount();

			managerInstance.Release(client);
		}
	}
}
