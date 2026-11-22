#pragma once
#include "IOCPEvent.h"
#include "Socket.h"

class CTCPAcceptor;

class CAcceptEvent : public CIOCPEvent
{
private:
	CTCPAcceptor*	m_owner = nullptr;
	CSocket			m_socket;
	byte			m_addrBuf[(sizeof(SOCKADDR_IN) + 16) * 2] = {};

public:
	CAcceptEvent() noexcept = default;
	~CAcceptEvent() noexcept override = default;

	void Cleanup() noexcept override;
	void Handle(IOCP_CONTEXT& _context, DWORD _byteTrans) override;
	void HandleError(IOCP_CONTEXT& _context, DWORD _error) override;

	bool	 Initialize(CTCPAcceptor* _owner) noexcept;
	CSocket& GetSocket() noexcept;
	CSocket	 TakeSocket() noexcept;
	byte*	 GetAddressBuffer() noexcept;
	DWORD	 GetAddressBufferSize() const noexcept;
	bool	 GetRemoteEndPoint(CIpEndPoint& _endPoint) noexcept;
};
