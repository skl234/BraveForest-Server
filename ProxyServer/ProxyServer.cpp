#include "ProxyService.h"
#include "ProxyServerConfig.h"
#include <clocale>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

int main()
{
	setlocale(LC_ALL, "");

	INI_CONFIG config{};
	if (!CProxyServerConfig::Load(config)) return 1;

	CProxyService service;
	if (!service.Initialize(&config)) return 1;
	if (!service.Run()) return 1;
	service.Stop();

	return 0;
}
