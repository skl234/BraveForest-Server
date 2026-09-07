#include "OverlappedManager.h"
#include "../Utility_Core/LockGuard.h"

void COverlappedManager::Initialize(uint64_t _size)
{
	InitializeSRWLock(&m_lock);

	m_list.reserve(_size);

	for (uint64_t i = 0; i < _size; ++i)
	{
		WSAOVERLAPPED_EX overlapped = {};
		m_list.push_back(WSAOVERLAPPED_EX(overlapped));
		m_pool.emplace_back(&m_list.back());
	}
}

WSAOVERLAPPED_EX* COverlappedManager::Acquire()
{
	WSAOVERLAPPED_EX* overlapped = nullptr;
	{
		CLockGuard guard(m_lock);
		if (m_pool.empty()) return nullptr;

		overlapped = m_pool.front();
		m_pool.pop_front();
	}
	if (!overlapped) return overlapped;

	*overlapped = {};
	return overlapped;
}

void COverlappedManager::Release(WSAOVERLAPPED_EX* _overlapped)
{
	if (!_overlapped) return;

	*_overlapped = {};

	CLockGuard guard(m_lock);
	m_pool.push_back(_overlapped);
}
