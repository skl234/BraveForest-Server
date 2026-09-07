#pragma once
#include "../Network_Core/INI_CONFIG.h"

struct LOGINSERVER_CONFIG : public INI_CONFIG
{
	std::string		zoneInfoPath;
	std::wstring	dbUser;
	std::wstring	dbPassword;
	std::wstring	accountDsn;
	uint64_t		accountConnectionCount;
	std::wstring	gameDsn;
	uint64_t		gameConnectionCount;
	uint64_t		packetTaskPoolSize;
};

class CLoginServerConfig
{
public:
	static bool Load(LOGINSERVER_CONFIG& _config, const char* _zoneInfoPath = nullptr);
};
