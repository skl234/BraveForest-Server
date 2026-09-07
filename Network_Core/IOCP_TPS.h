#pragma once
#include <WinSock2.h>
#include <cstdint>
struct IOCP_TPS
{
	uint64_t	index;
	uint64_t	total;
	uint64_t	recvCount;
	uint64_t	sendCount;
	uint64_t	acceptCount;
};
