#pragma once
#include "../Network_Core/INI_CONFIG.h"

struct LOGINSERVER_CONFIG : public INI_CONFIG
{
	std::wstring	dbUser;
	std::wstring	dbPassword;
	std::wstring	accountDsn;
	uint64_t		accountConnectionCount;
	uint64_t		packetTaskPoolSize;
};

class CLoginServerConfig
{
public:
	static bool Load(LOGINSERVER_CONFIG& _config);
};
