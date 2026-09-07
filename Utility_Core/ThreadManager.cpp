#include "ThreadManager.h"

CThreadManager::~CThreadManager()
{
	AllJoin();
}

void CThreadManager::AllJoin()
{
	size_t size = m_list.size();
	for (size_t i = 0; i < size; ++i)
	{
		std::thread& thread = m_list[i];
		if (thread.joinable()) thread.join();
	}
}
