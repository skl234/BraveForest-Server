#pragma once
#include <WinSock2.h>
#include <vector>
#include "../Common/BACK_SERVER_INFO.h"

struct INI_CONFIG
{
	std::string				ip;
	uint16_t				port;
	uint64_t				postAcceptSize;
	uint64_t				iocpThreadSize;
	std::string				logPath;
	uint64_t				logicThreadSize;

	//Proxy
	uint64_t					  maxClientConnection;
	std::vector<BACK_SERVER_INFO> backServerInfoList;

	//Login
};
