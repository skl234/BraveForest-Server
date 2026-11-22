#include "LoginServerConfig.h"
#include "../Utility_Core/INIFile.h"

bool CLoginServerConfig::Load(LOGINSERVER_CONFIG& _config)
{
	_config = {};
	_config.logPath = "Log.log";
	_config.iocpThreadSize = 6;
	_config.iocpConcurrency = 0;
	_config.ip = "127.0.0.1";
	_config.port = 30003;
	_config.postAcceptSize = 1;
	_config.maxClientConnection = 1;
	_config.logicThreadSize = 1;
	_config.accountConnectionCount = 1;
	_config.packetTaskPoolSize = 100000;

	_config.recvEventSize = 1;
	_config.sendEventSize = 1;
	_config.acceptEventSize = _config.postAcceptSize;

	std::filesystem::path directory = CINIFile::GetExecutableDirectory();
	if (directory.empty()) return false;
	CINIFile database;
	if (!database.Open(directory / L"Config" / L"Database.local.ini")) return false;
	_config.dbUser = database.ReadString(L"Database", L"User");
	_config.dbPassword = database.ReadString(L"Database", L"Password");
	_config.accountDsn = database.ReadString(L"Database", L"AccountDsn", L"account_db");

	return true;
}
