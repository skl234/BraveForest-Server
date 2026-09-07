#include "IOCP.h"

CIOCP::~CIOCP()
{
	Close();
}

bool CIOCP::Initialize(DWORD _threadSize)
{
	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _threadSize);
	if (!m_completionPort) return false;

	m_threadSize = _threadSize;

	return true;
}

bool CIOCP::Regist(HANDLE _handle, ULONG_PTR _completionKey)
{
	if (!CreateIoCompletionPort(_handle, m_completionPort, _completionKey, 0)) return false;
	
	return true;
}

bool CIOCP::GQCS(DWORD* _byteTrans, ULONG_PTR* _key, OVERLAPPED** _overlapped)
{
	return GetQueuedCompletionStatus(m_completionPort, _byteTrans, _key, _overlapped, INFINITE);
}

void CIOCP::Shutdown()
{
	for (DWORD i = 0; i < m_threadSize; ++i)
	{
		PostQueuedCompletionStatus(m_completionPort, 0, 0, nullptr);
	}
}

void CIOCP::Close()
{
	if(m_completionPort) CloseHandle(m_completionPort); 
	m_completionPort = NULL;
}

DWORD CIOCP::GetThreadSize()
{
	return m_threadSize;
}

//CIOCP::CIOCP():
//	m_completionPort(NULL)
//{
//}
//
//CIOCP::~CIOCP()
//{
//	Stop();
//	if (m_completionPort) { CloseHandle(m_completionPort); m_completionPort = NULL; }
//}
//
//bool CIOCP::Initialize(DWORD _workerSize)
//{
//	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _workerSize);
//	if (!m_completionPort) return false;
//
//	m_handleList.resize(_workerSize, NULL);
//	m_tpsList.resize(_workerSize, { NULL,NULL });
//	
//	return true;
//}
//
//bool CIOCP::Start()
//{
//	size_t size = m_handleList.size();
//	for (size_t i = 0; i < size; ++i)
//	{
//		uint32_t threadId = 0;
//		HANDLE handle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ThreadFunc, this, 0, &threadId));
//		if (!handle) return false;
//
//		m_handleList[i] = handle;
//		m_tpsList[i].threadId = threadId;
//	}
//
//	return true;
//}
//
//void CIOCP::Stop()
//{
//	//Stop 재호출 방지
//	if (m_handleList.empty() || !m_handleList[0]) return;
//
//	size_t size = m_handleList.size();
//	for (size_t i = 0; i < size; ++i)
//	{
//		PostQueuedCompletionStatus(m_completionPort, 0, 0, nullptr);
//	}
//
//	for (size_t i = 0; i < size; ++i)
//	{
//		HANDLE& handle = m_handleList[i];
//		if (!handle) continue;
//
//		WaitForSingleObject(handle, INFINITE);
//		CloseHandle(handle);
//		handle = NULL;
//	}
//}
//
//bool CIOCP::Add(HANDLE _handle, ULONG_PTR _completionKey)
//{
//	if (!CreateIoCompletionPort(_handle, m_completionPort, _completionKey, 0))
//	{
//		return false;
//	}
//
//	return true;
//}
//
//std::vector<IOCP_TPS> CIOCP::GetTPSList()
//{
//	std::vector<IOCP_TPS> temp = m_tpsList;
//	size_t size = m_tpsList.size();
//	for (size_t i = 0; i < size; ++i)
//	{
//		m_tpsList[i].tps = 0;
//		m_tpsList[i].tps_recv = 0;
//		m_tpsList[i].tps_send = 0;
//		m_tpsList[i].tps_accept = 0;
//	}
//
//	return temp;
//}
//
//void CIOCP::Work()
//{
//	uint64_t* tps = nullptr;
//	uint64_t* tps_recv = nullptr;
//	uint64_t* tps_send = nullptr;
//	uint64_t* tps_accept = nullptr;
//
//	DWORD threadID = GetCurrentThreadId();
//	size_t size = m_tpsList.size();
//	for (size_t i = 0; i < size; ++i)
//	{
//		if (m_tpsList[i].threadId != threadID) continue;
//
//		tps = &m_tpsList[i].tps;
//		tps_recv = &m_tpsList[i].tps_recv;
//		tps_send = &m_tpsList[i].tps_send;
//		tps_accept = &m_tpsList[i].tps_accept;
//	}
//
//	if (tps == nullptr) std::cout << "THREAD ID 감지 실패\n";
//
//
//	DWORD				byteTrans;
//	ULONG_PTR			key;
//	WSAOVERLAPPED_EX*	overlapped;
//
//	while (true)
//	{
//		bool result = GetQueuedCompletionStatus(m_completionPort, &byteTrans, &key, reinterpret_cast<LPOVERLAPPED*>(&overlapped), INFINITE);
//		
//		if (!overlapped)
//		{
//			if(!key) break; //PostQueuedCompletionStatus(종료신호)
//
//			//overlapped가 NULL이 나오고 key가 null이 아닌 경우가 있을까?
//			CLogManager::GetInstance().Write(eLogLevel::eERROR, 
//				"iocp overlapped||key error : %d", 
//				WSAGetLastError());
//			continue;
//		}
//
//		DWORD error;
//		if (!result) error = GetLastError();
//
//		++(*tps);
//		if (overlapped->type == eOverlappedType::Recv) ++(*tps_recv);
//		if (overlapped->type == eOverlappedType::Send) ++(*tps_send);
//		if (overlapped->type == eOverlappedType::Accept) ++(*tps_accept);
//
//		switch (overlapped->type)
//		{
//		case eOverlappedType::Accept:
//			if (result) reinterpret_cast<CTCPAcceptor*>(overlapped->owner)->OnAcceptComplete(overlapped, byteTrans);
//			else reinterpret_cast<CTCPAcceptor*>(overlapped->owner)->HandleError(overlapped, error);
//			break;
//
//		case eOverlappedType::Recv:
//			if (result) reinterpret_cast<CTCPConnection*>(overlapped->owner)->OnRecvComplete(overlapped, byteTrans);
//			else reinterpret_cast<CTCPConnection*>(overlapped->owner)->HandleError(overlapped, error);
//			break;
//
//		case eOverlappedType::Send:
//			if (result) reinterpret_cast<CTCPConnection*>(overlapped->owner)->OnSendComplete(overlapped, byteTrans);
//			else reinterpret_cast<CTCPConnection*>(overlapped->owner)->HandleError(overlapped, error);
//			break;
//
//		case eOverlappedType::Client_Recv:
//			if (result) reinterpret_cast<CTCPClient*>(overlapped->owner)->OnRecvComplete(overlapped, byteTrans);
//			else reinterpret_cast<CTCPClient*>(overlapped->owner)->HandleError(overlapped, error);
//			break;
//
//		default:
//			break;
//		}
//	}
//}
