// DummyReConnect.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#ifdef _DEBUG
#pragma comment(lib, "../x64/Debug/Utility_Core.lib")
#pragma comment(lib, "../x64/Debug/ODBC_Core.lib")
#pragma comment(lib, "../x64/Debug/Network_Core.lib")
#else
#pragma comment(lib, "../x64/Release/Utility_Core.lib")
#pragma comment(lib, "../x64/Release/ODBC_Core.lib")
#pragma comment(lib, "../x64/Release/Network_Core.lib")
#endif

#include <WinSock2.h>
#include <iostream>
#include <string>
#include "DummyApp.h"

int main()
{
    std::string ip;
    uint16_t port;
    uint16_t threadCount;
    uint32_t dummyCount;
    int64_t interval;
    int64_t per;

    //std::cout << "IP 입력: ";
    //std::cin >> ip;
    ip = "127.0.0.1";

    //std::cout << "Port 입력: ";
    //std::cin >> port;
    port = 30002;

    std::cout << "Thread Count : ";
    std::cin >> threadCount;

    std::cout << "Thread Interval : ";
    std::cin >> interval;

    std::cout << "Thread Send Count Per Interval : ";
    std::cin >> per;

    std::cout << "Dummy Count : ";
    std::cin >> dummyCount;

    CDummyApp app;
    app.Initialize(ip, port, dummyCount);
    app.Run(threadCount, interval, per);
    app.Stop();

    return 0;
};
