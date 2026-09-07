#pragma once
#include <WinSock2.h>
#include <cstdint>

struct JOB_TPS
{
	uint64_t	threadId;
	uint64_t	tps;
};
