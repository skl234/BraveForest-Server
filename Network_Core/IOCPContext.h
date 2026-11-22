#pragma once
#include <WinSock2.h>

class CIOCPEvent;

struct IOCP_CONTEXT
{
	WSAOVERLAPPED	overlapped = {};
	CIOCPEvent*		event = nullptr;
};
