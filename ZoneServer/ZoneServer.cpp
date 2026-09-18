#include "ZoneServerService.h"
#include "ZoneServerConfig.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "odbc32.lib")

int main()
{
	ZONESERVER_CONFIG config{};
	if (!CZoneServerConfig::Load(config)) return 0;

	CZoneServerService service;
	if (!service.Initialize(&config)) return 0;
	if (!service.Run()) return 0;
	service.Stop();

	return 0;
}
