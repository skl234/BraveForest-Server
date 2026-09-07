#include "LoginService.h"
#include "LoginServerConfig.h"
#include <clocale>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "odbc32.lib")

int main(int _argc, char* _argv[])
{
	setlocale(LC_ALL, "");

	const char* path = nullptr;
	if (_argc > 1) path = _argv[1];

	LOGINSERVER_CONFIG config{};
	if (!CLoginServerConfig::Load(config, path)) return 1;

	CLoginService service;
	if (!service.Initialize(&config)) return 1;
	if (!service.Run()) return 1;
	service.Stop();

	return 0;
}
