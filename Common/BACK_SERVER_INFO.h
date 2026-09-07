#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>

enum class eBackServerType : uint16_t
{
	None = 0,
	Login,
	Zone,

	End,
};

struct BACK_SERVER_INFO
{
	eBackServerType type;
	uint64_t        serverId;
	std::string		ip;
	uint16_t		port;
};
