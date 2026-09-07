#pragma once
#include <WinSock2.h>
#include <atomic>
#include "../Utility_Core/Singleton.h"
#include <string>

class CDummyAppRegistry : public CSingleton<CDummyAppRegistry>
{
	friend class CSingleton<CDummyAppRegistry>;

private:
	std::string m_ip;
	uint16_t    m_port;

private:
	explicit CDummyAppRegistry() = default;
	~CDummyAppRegistry() = default;

public:
	void Initialize(const std::string& _ip, uint16_t _port);
	const std::string& GetIP();
	uint16_t GetPort();
};

