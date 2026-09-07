#pragma once
#include "../Utility_Core/Thread.h"
#include <atomic>

class CDummyScheduler : public CThread
{
private:
	std::atomic<bool> m_run = true;
	int64_t			  m_interval;
	int64_t			  m_sendCountPerInterval;

public:
	CDummyScheduler(int64_t _interval, int64_t _sendCountPerInterval);
	~CDummyScheduler() override;

protected:
	void Run() override;
};
