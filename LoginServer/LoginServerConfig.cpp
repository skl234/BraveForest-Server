#include "LoginServerConfig.h"
#include "../Utility_Core/INIFile.h"

bool CLoginServerConfig::Load(LOGINSERVER_CONFIG& _config, const char* _zoneInfoPath)
{
	_config = {};
	_config.logPath = "Log.log";
	_config.iocpThreadSize = 6;
	_config.ip = "192.168.219.100";
	_config.port = 30003;
	_config.postAcceptSize = 1;
	_config.maxClientConnection = 1;
	_config.logicThreadSize = 1;
	_config.accountConnectionCount = 1;
	_config.gameConnectionCount = 1;
	_config.packetTaskPoolSize = 100000;

	std::filesystem::path directory = CINIFile::GetExecutableDirectory();
	if (directory.empty()) return false;
	CINIFile database;
	if (!database.Open(directory / L"Config" / L"Database.local.ini")) return false;
	_config.dbUser = database.ReadString(L"Database", L"User");
	_config.dbPassword = database.ReadString(L"Database", L"Password");
	_config.accountDsn = database.ReadString(L"Database", L"AccountDsn", L"account_db");
	_config.gameDsn = database.ReadString(L"Database", L"GameDsn", L"game_db");

	std::filesystem::path path = directory / L"Data" / L"ZoneInfo.txt";
	if (_zoneInfoPath != nullptr) path = _zoneInfoPath;
	else if (!std::filesystem::exists(path))
	{
		// VS에서 실행할 때는 개발용 데이터 경로 사용
		std::wstring name = directory.filename().wstring();
		if (name == L"Debug" || name == L"Release")
		{
			path = L"Data/ZoneInfo.txt";
			if (!std::filesystem::exists(path)) path = L"../Data/ZoneInfo.txt";
		}
	}
	_config.zoneInfoPath = std::filesystem::absolute(path).lexically_normal().string();
	return true;
}
