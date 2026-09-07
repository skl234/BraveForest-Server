#pragma once
#include <WinSock2.h>
#include "../Utility_Core/Singleton.h"
#include <atomic>
#include <cstdint>

class CDummyMonitor : public CSingleton<CDummyMonitor>
{
	friend class CSingleton<CDummyMonitor>;

private:
	std::atomic<int64_t> m_sendCount = 0;
	std::atomic<int64_t> m_sendFailCount = 0;
	std::atomic<int64_t> m_recvCount = 0;


private:
	CDummyMonitor() = default;
	~CDummyMonitor() = default;

public:
	void ShowMonitor();
	void AddSendCount();
	void AddSendFailCount();
	void AddRecvCount();
	void Reset();
};
