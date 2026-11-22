#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>

class CIpEndPoint
{
private:
	SOCKADDR_IN m_addr = {};

public:
	CIpEndPoint() noexcept = default;
	explicit CIpEndPoint(const SOCKADDR_IN& _addr) noexcept;
	~CIpEndPoint() = default;

	bool				Initialize(const std::string& _ip, const uint16_t _port) noexcept;
	std::string			GetIp() const;
	uint16_t			GetPort() const noexcept;
	const SOCKADDR_IN&	GetAddress() const noexcept;
};
