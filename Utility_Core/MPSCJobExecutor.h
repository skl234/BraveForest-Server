#pragma once
#include <WinSock2.h>
#include <thread>
#include <atomic>
#include "SwapQueue.h"
#include "Job.h"
#include "JOB_TPS.h"

class CMPSCJobExecutor
{
private:
	CMPSCQueue<CJob*>		m_queue;
	HANDLE				    m_hEvent = NULL;

	std::thread				m_thread;
	std::atomic_bool		m_active = false;
	JOB_TPS					m_tps = { 0, };

public:
	CMPSCJobExecutor() = default;
	~CMPSCJobExecutor();

	bool Initialize();
	bool Start();
	void Stop();
	void Add(CJob* _job);

	uint64_t GetSize();
	JOB_TPS GetTPS();

private:
	void LoopExecuteJob();
};
