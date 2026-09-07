#include "ZoneServerConfig.h"
#include "../Utility_Core/INIFile.h"

bool CZoneServerConfig::Load(ZONESERVER_CONFIG& _config, const char* _configPath)
{
	_config = {};
	std::filesystem::path directory = CINIFile::GetExecutableDirectory();
	if (directory.empty()) return false;

	CINIFile database;
	if (!database.Open(directory / L"Config" / L"Database.local.ini")) return false;
	_config.dbUser = database.ReadString(L"Database", L"User");
	_config.dbPassword = database.ReadString(L"Database", L"Password");
	_config.gameDsn = database.ReadString(L"Database", L"GameDsn", L"game_db");

	std::filesystem::path path = directory / L"Config" / L"ZoneServer.ini";
	if (_configPath != nullptr)
		path = _configPath;
	else if (!std::filesystem::exists(path))
	{
		// VS에서 실행할 때는 개발용 설정 경로 사용
		std::wstring name = directory.filename().wstring();
		if (name == L"Debug" || name == L"Release")
		{
			path = L"ZoneServer.ini";
			if (!std::filesystem::exists(path)) path = L"ZoneServer/ZoneServer.ini";
		}
	}

	CINIFile zone;
	if (!zone.Open(path)) return false;
	if (!zone.ReadPath(L"Zone", L"ZoneInfoPath", _config.zoneInfoPath)) return false;
	if (!zone.ReadPath(L"Zone", L"FieldInfoPath", _config.fieldInfoPath)) return false;
	if (!zone.ReadPath(L"Zone", L"MonsterSpawnPath", _config.monsterSpawnPath)) return false;

	_config.logPath = "ZoneServer.log";
	_config.iocpThreadSize = zone.ReadNumber(L"Zone", L"IOCPThreadSize", 6);
	std::wstring ip = zone.ReadString(L"Zone", L"IP", L"192.168.219.100");
	for (uint64_t i = 0; i < ip.size(); ++i)
	{
		if (ip[i] > 127) return false;
		_config.ip.push_back(static_cast<char>(ip[i]));
	}
	_config.port = static_cast<uint16_t>(zone.ReadNumber(L"Zone", L"Port", 30005));
	_config.postAcceptSize = 1;
	_config.maxClientConnection = 1;
	_config.zoneId = zone.ReadNumber(L"Zone", L"ZoneId", 2);
	_config.channelCount = zone.ReadNumber(L"Channel", L"ChannelCount", 10);
	_config.maxPlayerPerChannel = zone.ReadNumber(L"Channel", L"MaxPlayerPerChannel", 256);
	_config.maxMonsterPerChannel = zone.ReadNumber(L"Channel", L"MaxMonsterPerChannel", 512);
	_config.dbConnectionCount = zone.ReadNumber(L"Database", L"ConnectionCount", static_cast<uint32_t>(_config.channelCount));
	_config.packetTaskPoolSize = 65536;
	return true;
}
