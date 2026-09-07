#pragma once
#include "../Network_Core/INI_CONFIG.h"
#include "SERVER_CONNECTION_INFO.h"
#include <vector>

struct PROXYSERVER_CONFIG : public INI_CONFIG
{
	std::string				ip;
	uint16_t				port;
	uint16_t				postAcceptCount;

	uint64_t				maxClientConnection;
	uint64_t				packetHandleWorkerCount;

	std::vector<SERVER_CONNECTION_INFO> backServerInfoList;
};
