#pragma once
#include <WinSock2.h>
#include <mutex>
#include "UtilityMacros.h"

enum class eLockType : byte
{
	CS = 0,
	SRWLOCK,
	MUTEX,
};

enum class eLockMode : byte
{
	Exclusive = 0,
	Shared,
};

class CLockGuard
{
private:
	void*	  m_lock = nullptr;
	eLockType m_type;
	eLockMode m_mode = eLockMode::Exclusive;

public:
	explicit CLockGuard(CRITICAL_SECTION& _cs);
	explicit CLockGuard(SRWLOCK& _srwLock, eLockMode _mode = eLockMode::Exclusive);
	explicit CLockGuard(std::mutex& _mutex);
	~CLockGuard();

	DELETE_COPY_MOVE(CLockGuard);
};
