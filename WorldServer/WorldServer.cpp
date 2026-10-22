// WorldServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "WorldService.h"
#include "WorldServerConfig.h"
#include <clocale>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "odbc32.lib")

//이거.....WorldServer보다 CommunityServer 가 더 잘 어울리는 서버명 인가?
//WorldServer Zone이동간, 아 그러면 로그인 성공시 캐릭터선택창에서 World
//proxy는 진짜로 toss기능만
//아 그러면 어디 zone에 있는지도 world에서 관리하면 확실히 깔끔해지겠네
//어디 zone에 있는지 지금 proxy가 관리해버리고 있다.
//

int main()
{
	setlocale(LC_ALL, "");

	WORLDSERVER_CONFIG config{};
	if (!CWorldServerConfig::Load(config)) return 0;

	WorldService service;
	if (!service.Initialize(&config)) return 0;
	if (!service.Run())
	{
		service.Stop();
		return 0;
	}
	service.Stop();

	return 0;
}
