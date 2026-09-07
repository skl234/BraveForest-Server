#include "IOCPServer.h"
#include "OverlappedEx.h"
#include "TCPConnection.h"
#include "../Utility_Core/LockGuard.h"

CIOCPServer::CIOCPServer()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	//if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) throw 0;
}

CIOCPServer::~CIOCPServer()
{
	WSACleanup();
}

bool CIOCPServer::Initialize(DWORD _threadSize, uint64_t _overlappedSize)
{
	if (!m_iocp.Initialize(_threadSize)) return false;

	m_overlappedManager.Initialize(_overlappedSize);

	return true;
}

void CIOCPServer::Start()
{
	DWORD threadSize = m_iocp.GetThreadSize();
	for (DWORD i = 0; i < threadSize; ++i)
	{
		m_threadManager.Add([this]() { LoopGQCS(); });
	}
}

void CIOCPServer::Stop()
{
	m_iocp.Shutdown();
	m_threadManager.AllJoin();
	m_iocp.Close();
	WSACleanup();
}

CIOCP& CIOCPServer::GetIOCP()
{
	return m_iocp;
}

COverlappedManager& CIOCPServer::GetOverlappedManager()
{
	return m_overlappedManager;
}

std::vector<IOCP_TPS> CIOCPServer::GetTPSList()
{
	return m_tpsManager.GetList();
}

void CIOCPServer::LoopGQCS()
{
	uint64_t index = m_tpsManager.RegisterIndex();

	DWORD				byteTrans;
	ULONG_PTR			key;
	WSAOVERLAPPED_EX*	overlapped;

	while (true)
	{
		bool result = m_iocp.GQCS(&byteTrans, &key, reinterpret_cast<LPOVERLAPPED*>(&overlapped));

		if (!overlapped || !key) break;

		m_tpsManager.Update(index, overlapped->type);
	
		DWORD error;
		if (!result) error = GetLastError();

		switch (overlapped->type)
		{
		case eOverlappedType::Accept:
			if (result) reinterpret_cast<CTCPAcceptor*>(overlapped->owner)->HandleAccept(overlapped, byteTrans);
			else reinterpret_cast<CTCPAcceptor*>(overlapped->owner)->HandleError(overlapped, error);
			break;

		case eOverlappedType::Recv:
			if (result) reinterpret_cast<CTCPConnection*>(overlapped->owner)->HandleRecv(overlapped, byteTrans);
			else reinterpret_cast<CTCPConnection*>(overlapped->owner)->HandleError(overlapped, error);
			break;

		case eOverlappedType::Send:
			if (result) reinterpret_cast<CTCPConnection*>(overlapped->owner)->HandleSend(overlapped, byteTrans);
			else reinterpret_cast<CTCPConnection*>(overlapped->owner)->HandleError(overlapped, error);
			break;

		default:
			break;
		}
	}
}
