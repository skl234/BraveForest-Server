#pragma once
#include "../Network_Core/INI_CONFIG.h"

struct ZONESERVER_CONFIG : public INI_CONFIG
{
	uint64_t		zoneId;
	uint64_t		channelCount;
	uint64_t		maxPlayerPerChannel;
	uint64_t		maxMonsterPerChannel;
	std::string		fieldInfoPath;
	std::string		zoneInfoPath;
	std::string		monsterSpawnPath;
	std::wstring	dbUser;
	std::wstring	dbPassword;
	std::wstring	gameDsn;
	uint64_t		dbConnectionCount;
	uint64_t		packetTaskPoolSize;
};

class CZoneServerConfig
{
public:
	static bool Load(ZONESERVER_CONFIG& _config, const char* _configPath = nullptr);
};
