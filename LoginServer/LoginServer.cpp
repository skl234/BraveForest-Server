#include "LoginService.h"
#include "LoginServerConfig.h"
#include <clocale>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "odbc32.lib")

int main()
{
	setlocale(LC_ALL, "");

	LOGINSERVER_CONFIG config{};
	if (!CLoginServerConfig::Load(config)) return 0;

	CLoginService service;
	if (!service.Initialize(&config)) return 0;
	if (!service.Run()) return 0;
	service.Stop();

	return 0;
}
