#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "../Utility_Core/UtilityMacros.h"

class CIOCP
{
private:
	HANDLE m_completionPort = NULL;
	DWORD  m_threadSize = NULL;

public:
	CIOCP() = default;
	~CIOCP();

	DELETE_COPY_MOVE(CIOCP);

	bool Initialize(DWORD _threadSize);
	bool Regist(HANDLE _handle, ULONG_PTR _completionKey);
	bool GQCS(DWORD* _byteTrans, ULONG_PTR* _key, OVERLAPPED** _overlapped);

	void Shutdown();
	void Close();

	DWORD GetThreadSize();
};
//
//struct IOCP_TPS
//{
//	DWORD		threadId;
//	uint64_t	tps;
//	uint64_t	tps_recv;
//	uint64_t	tps_send;
//	uint64_t	tps_accept;
//};
//
//class CIOCP : public CSingleton<CIOCP>, public IWorker
//{
//	friend class CSingleton<CIOCP>;
//
//private:
//	HANDLE					m_completionPort;
//	std::vector<HANDLE>		m_handleList;
//	std::vector<IOCP_TPS>	m_tpsList;
//
//private:
//	CIOCP();
//	~CIOCP();
//
//public:
//	//=============================================
//	//override
//	bool Initialize(DWORD _workerSize) override;
//	bool Start() override;
//	void Stop() override;
//	//=============================================
//
//	bool Add(HANDLE _handle, ULONG_PTR _completionKey);
//	
//	std::vector<IOCP_TPS> GetTPSList();
//
//protected:
//	//override
//	void Work() override;
//};
