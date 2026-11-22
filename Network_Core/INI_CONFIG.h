#pragma once
#include <WinSock2.h>
#include <vector>
#include <string>
#include <cstdint>
#include "../Common/BACK_SERVER_INFO.h"

struct INI_CONFIG
{
	std::string				ip;
	uint16_t				port;
	uint64_t				postAcceptSize;
	uint64_t				iocpThreadSize;
	std::string				logPath;
	uint64_t				logicThreadSize;

	// Network_Core
	DWORD					iocpConcurrency = 0;
	uint64_t				recvEventSize = 0;
	uint64_t				sendEventSize = 0;
	uint64_t				acceptEventSize = 0;

	//Proxy
	uint64_t					  maxClientConnection;
	std::vector<BACK_SERVER_INFO> backServerInfoList;

	//Login
};
