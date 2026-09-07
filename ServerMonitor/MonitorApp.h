#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <memory>

class CMonitorApp
{
private:
	SOCKET		m_socket;
	SOCKADDR_IN m_addr;
	char		m_recvBuff[2048];
	uint16_t	m_offset = 0;

	uint64_t	m_sendPending = 0;

public:
	explicit CMonitorApp();
	~CMonitorApp() = default;

	bool Initialize();
	void Run();
	void Stop();

private:
	void ShowMonitor();
};
