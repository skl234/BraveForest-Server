#pragma once
#include <WinSock2.h>
#include <memory>
#include <vector>
#include "IOCP.h"
#include "TCPAcceptor.h"
#include "OverlappedManager.h"
#include "IOCPTPSManager.h"
#include "../Utility_Core/ThreadManager.h"
#include "../Utility_Core/UtilityMacros.h"

class CIOCPServer
{
private:
	CIOCP				m_iocp;
	COverlappedManager	m_overlappedManager;

	CIOCPTPSManager		m_tpsManager;
	CThreadManager 		m_threadManager;

public:
	CIOCPServer();
	~CIOCPServer();

	DELETE_COPY_MOVE(CIOCPServer);

	bool Initialize(DWORD _threadSize, uint64_t _overlappedSize);
	void Start();
	void Stop();
	
	CIOCP&				  GetIOCP();
	COverlappedManager&   GetOverlappedManager();
	std::vector<IOCP_TPS> GetTPSList();

private:
	void	  LoopGQCS();
};
