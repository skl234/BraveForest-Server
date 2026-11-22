#include "IpEndPoint.h"
#include <WS2tcpip.h>

CIpEndPoint::CIpEndPoint(const SOCKADDR_IN& _addr) noexcept:
	m_addr(_addr)
{
}

bool CIpEndPoint::Initialize(const std::string& _ip, const uint16_t _port) noexcept
{
	SOCKADDR_IN addr = {};

	if (inet_pton(AF_INET, _ip.c_str(), &addr.sin_addr) != 1)
		return false;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	m_addr = addr;

	return true;
}

std::string CIpEndPoint::GetIp() const
{
	if (m_addr.sin_family != AF_INET)
		return {};

	char ip[INET_ADDRSTRLEN] = {};
	if (inet_ntop(AF_INET, &m_addr.sin_addr, ip, sizeof(ip)) == nullptr)
		return {};

	return ip;
}

uint16_t CIpEndPoint::GetPort() const noexcept
{
	return ntohs(m_addr.sin_port);
}

const SOCKADDR_IN& CIpEndPoint::GetAddress() const noexcept
{
	return m_addr;
}
