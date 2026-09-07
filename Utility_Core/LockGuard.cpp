#include "LockGuard.h"

CLockGuard::CLockGuard(CRITICAL_SECTION& _cs):
	m_lock(&_cs),
	m_type(eLockType::CS)
{
	EnterCriticalSection(&_cs);
}

CLockGuard::CLockGuard(SRWLOCK& _srwLock, eLockMode _mode):
	m_lock(&_srwLock),
	m_type(eLockType::SRWLOCK),
	m_mode(_mode)
{
	if (m_mode == eLockMode::Exclusive)
	{
		AcquireSRWLockExclusive(&_srwLock);
	}
	else
	{
		AcquireSRWLockShared(&_srwLock);
	}
}

CLockGuard::CLockGuard(std::mutex& _mutex):
	m_lock(&_mutex),
	m_type(eLockType::MUTEX)
{
	_mutex.lock();
}

CLockGuard::~CLockGuard()
{
	switch (m_type)
	{
	case eLockType::CS:
		LeaveCriticalSection(static_cast<CRITICAL_SECTION*>(m_lock));
		break;

	case eLockType::SRWLOCK:
		if (m_mode == eLockMode::Exclusive)
		{
			ReleaseSRWLockExclusive(static_cast<SRWLOCK*>(m_lock));
		}
		else
		{
			ReleaseSRWLockShared(static_cast<SRWLOCK*>(m_lock));
		}
		break;

	case eLockType::MUTEX:
		static_cast<std::mutex*>(m_lock)->unlock();
		break;
	}
}
