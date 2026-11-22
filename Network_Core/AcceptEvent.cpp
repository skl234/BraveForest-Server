#include "AcceptEvent.h"
#include <cassert>
#include "IOCPContext.h"
#include "TCPAcceptor.h"
#include <MSWSock.h>
#include <cstring>
#include <utility>

void CAcceptEvent::Cleanup() noexcept
{
	m_owner = nullptr;
	m_socket.Close();
	memset(m_addrBuf, 0, sizeof(m_addrBuf));
}

void CAcceptEvent::Handle(IOCP_CONTEXT& _context, DWORD _byteTrans)
{
	assert(m_owner != nullptr);
	if (_context.event != this || m_owner == nullptr)
		return;

	CTCPAcceptor* owner = m_owner;
	owner->HandleAccept(*this);
	owner->OnCompleteIO();
}

void CAcceptEvent::HandleError(IOCP_CONTEXT& _context, DWORD _error)
{
	assert(m_owner != nullptr);
	if (_context.event != this || m_owner == nullptr)
		return;

	CTCPAcceptor* owner = m_owner;
	owner->HandleError(*this, _error);
	owner->OnCompleteIO();
}


bool CAcceptEvent::Initialize(CTCPAcceptor* _owner) noexcept
{
	if (_owner == nullptr || m_owner != nullptr || m_socket.IsValid())
		return false;
	if (!m_socket.Initialize())
		return false;

	m_owner = _owner;

	return true;
}

CSocket& CAcceptEvent::GetSocket() noexcept
{
	return m_socket;
}

CSocket CAcceptEvent::TakeSocket() noexcept
{
	return std::move(m_socket);
}

byte* CAcceptEvent::GetAddressBuffer() noexcept
{
	return m_addrBuf;
}

DWORD CAcceptEvent::GetAddressBufferSize() const noexcept
{
	return static_cast<DWORD>(sizeof(m_addrBuf));
}

bool CAcceptEvent::GetRemoteEndPoint(CIpEndPoint& _endPoint) noexcept
{
	SOCKADDR* localAddr = nullptr;
	SOCKADDR* remoteAddr = nullptr;
	int localSize = 0;
	int remoteSize = 0;
	DWORD addrSize = sizeof(SOCKADDR_IN) + 16;

	GetAcceptExSockaddrs(m_addrBuf, 0, addrSize, addrSize, &localAddr, &localSize, &remoteAddr, &remoteSize);
	if (remoteAddr == nullptr || remoteSize < static_cast<int>(sizeof(SOCKADDR_IN))) return false;

	SOCKADDR_IN addr = {};
	memcpy(&addr, remoteAddr, sizeof(addr));
	if (addr.sin_family != AF_INET) return false;

	_endPoint = CIpEndPoint(addr);
	return true;
}
