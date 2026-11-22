#include "ProxyServerConfig.h"

bool CProxyServerConfig::Load(INI_CONFIG& _config)
{
	_config = {};
	_config.logPath = "Log.log";
	_config.iocpThreadSize = 12;
	_config.iocpConcurrency = 0;
	_config.ip = "127.0.0.1";
	_config.port = 30002;
	_config.postAcceptSize = 500;
	_config.maxClientConnection = 100000;
	_config.logicThreadSize = 24;

	_config.backServerInfoList.push_back({ eBackServerType::Login, 0, "127.0.0.1", 30003 });
	_config.backServerInfoList.push_back({ eBackServerType::World, 0, "127.0.0.1", 30010 });
	_config.backServerInfoList.push_back({ eBackServerType::Zone, 1, "127.0.0.1", 30004 });
	_config.backServerInfoList.push_back({ eBackServerType::Zone, 2, "127.0.0.1", 30005 });

	uint64_t serverConnectionSize = static_cast<uint64_t>(_config.backServerInfoList.size());
	_config.recvEventSize = _config.maxClientConnection + serverConnectionSize;
	_config.sendEventSize = _config.maxClientConnection + serverConnectionSize;
	_config.acceptEventSize = _config.postAcceptSize;
	return true;
}
