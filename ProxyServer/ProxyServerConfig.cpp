#include "ProxyServerConfig.h"

bool CProxyServerConfig::Load(INI_CONFIG& _config)
{
	_config = {};
	_config.logPath = "Log.log";
	_config.iocpThreadSize = 12;
	_config.ip = "192.168.219.100";
	_config.port = 30002;
	_config.postAcceptSize = 500;
	_config.maxClientConnection = 100000;
	_config.logicThreadSize = 24;

	_config.backServerInfoList.push_back({ eBackServerType::Login, 0, "192.168.219.100", 30003 });
	_config.backServerInfoList.push_back({ eBackServerType::Zone, 1, "192.168.219.100", 30004 });
	_config.backServerInfoList.push_back({ eBackServerType::Zone, 2, "192.168.219.100", 30005 });
	return true;
}
