#include "MPSCJobExecutor.h"

CMPSCJobExecutor::~CMPSCJobExecutor()
{
	Stop();
	if (m_hEvent) { CloseHandle(m_hEvent); m_hEvent = NULL; }
}

bool CMPSCJobExecutor::Initialize()
{
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!m_hEvent) return false;

	return true;
}

bool CMPSCJobExecutor::Start()
{
	if (m_hEvent == NULL) return false;
	if (m_thread.joinable()) return false;

	m_active.store(true, std::memory_order_relaxed);

	m_thread = std::thread([this]() { LoopExecuteJob(); });

	return true;
}

void CMPSCJobExecutor::Stop()
{
	m_active.store(false, std::memory_order_relaxed);

	if (m_hEvent) SetEvent(m_hEvent);

	if (m_thread.joinable()) m_thread.join();
}

void CMPSCJobExecutor::Add(CJob* _job)
{
	if (_job == nullptr) return;
	if (m_hEvent == NULL) return;

	m_queue.Push(_job);
	SetEvent(m_hEvent);
}

uint64_t CMPSCJobExecutor::GetSize()
{
	return m_queue.GetSize();
}

JOB_TPS CMPSCJobExecutor::GetTPS()
{
	JOB_TPS temp = m_tps;
	m_tps.tps = 0;
	return temp;
}

void CMPSCJobExecutor::LoopExecuteJob()
{
	m_tps.threadId = GetCurrentThreadId();

	while (m_active.load(std::memory_order_relaxed))
	{
		WaitForSingleObject(m_hEvent, INFINITE);

		while (true)
		{
			m_queue.Swap();

			while (true)
			{
				CJob* job = m_queue.Pop();
				if (job == nullptr) break;

				job->Execute();
				++m_tps.tps;
			}

			Sleep(0);
			if (m_queue.GetSize() == 0) break;
		}
	}
}
