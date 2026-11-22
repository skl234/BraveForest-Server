#include "WorldServerConfig.h"
#include "../Utility_Core/INIFile.h"

bool CWorldServerConfig::Load(WORLDSERVER_CONFIG& _config, const char* _zoneInfoPath)
{
	_config = {};
	_config.ip = "127.0.0.1";
	_config.port = 30010;
	_config.iocpThreadSize = 1;
	_config.iocpConcurrency = 0;
	_config.logicThreadSize = 1;
	_config.postAcceptSize = 1;
	_config.maxClientConnection = 1;
	_config.gameConnectionCount = 1;

	_config.recvEventSize = 1;
	_config.sendEventSize = 1;
	_config.acceptEventSize = _config.postAcceptSize;

	const std::filesystem::path directory = CINIFile::GetExecutableDirectory();
	if (directory.empty()) return false;
	CINIFile database;
	if (!database.Open(directory / L"Config" / L"Database.local.ini")) return false;
	_config.dbUser = database.ReadString(L"Database", L"User");
	_config.dbPassword = database.ReadString(L"Database", L"Password");
	_config.gameDsn = database.ReadString(L"Database", L"GameDsn", L"game_db");

	std::filesystem::path path = directory / L"Data" / L"ZoneInfo.txt";
	if (_zoneInfoPath != nullptr) path = _zoneInfoPath;
	_config.zoneInfoPath = std::filesystem::absolute(path).lexically_normal().string();
	return true;
}
