#pragma once
#include <WinSock2.h>
#include <memory>
#include <cstdint>
#include <vector>
#include <deque>
#include "OverlappedEx.h"
#include "../Utility_Core/UtilityMacros.h"

class COverlappedManager
{
private:
	std::vector<WSAOVERLAPPED_EX> m_list;
	std::deque<WSAOVERLAPPED_EX*> m_pool;
	SRWLOCK						  m_lock;

public:
	COverlappedManager() = default;
	~COverlappedManager() = default;

	DELETE_COPY_MOVE(COverlappedManager);

	void				Initialize(uint64_t _size);
	WSAOVERLAPPED_EX*	Acquire();
	void				Release(WSAOVERLAPPED_EX* _overlapped);
};
