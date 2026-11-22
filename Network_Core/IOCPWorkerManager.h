#pragma once
#include <WinSock2.h>
#include <vector>
#include <thread>
#include "IOCP.h"
#include "../Utility_Core/UtilityMacros.h"

class CIOCPWorkerManager
{
public:
	DELETE_COPY_MOVE(CIOCPWorkerManager);

private:
	std::vector<std::thread>	m_workerList;
	bool						m_isWork = false;

public:
	CIOCPWorkerManager() noexcept = default;
	~CIOCPWorkerManager() = default;

	// 초기화와 스레드 생성 예외는 Service에서 처리한다.
	bool Initialize(const uint32_t _threadSize);
	bool StartWorker(const CIOCP& _iocp);
	bool AllJoin(const CIOCP& _iocp);
	bool IsRunning() const noexcept;

private:
	void WorkerThreadProc(const CIOCP& _iocp);

	//void LoopGQCS();
	//void Worker(const CIOCP& _iocp);
	//Work
	//WorkerThreadProc
	//WorkerLoop
};
