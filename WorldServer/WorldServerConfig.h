#pragma once
#include "../Network_Core/INI_CONFIG.h"

struct WORLDSERVER_CONFIG : public INI_CONFIG
{
	std::string		zoneInfoPath;
	std::wstring	dbUser;
	std::wstring	dbPassword;
	std::wstring	gameDsn;
	uint64_t		gameConnectionCount;
};

class CWorldServerConfig
{
public:
	static bool	Load(WORLDSERVER_CONFIG& _config, const char* _zoneInfoPath = nullptr);
};
