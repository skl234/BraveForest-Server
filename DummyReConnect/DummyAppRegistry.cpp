#include "DummyAppRegistry.h"
#include "../Utility_Core/RandomNumberGenerator.h"

void CDummyAppRegistry::Initialize(const std::string& _ip, uint16_t _port)
{
	m_ip = _ip;
	m_port = _port;
}

const std::string& CDummyAppRegistry::GetIP()
{
	return m_ip;
}

uint16_t CDummyAppRegistry::GetPort()
{
	return m_port;
}
