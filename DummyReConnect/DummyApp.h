#pragma once
#include <WinSock2.h>
#include "../Network_Core/IOCP.h"
#include <vector>
#include <memory>
#include <string>

class CDummyApp
{
private:

public:
	CDummyApp();
	~CDummyApp();

	bool Initialize(std::string& _ip, uint16_t _port, uint32_t _dummyCount);
	void Run(uint16_t _threadCount, int64_t _interval, int64_t _per);
	void Stop();
};
