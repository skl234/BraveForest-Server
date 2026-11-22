#pragma once
#include <WinSock2.h>
#include "IpEndPoint.h"
#include "../Utility_Core/UtilityMacros.h"

class CSocket
{
public:
	DELETE_COPY(CSocket);

private:
	SOCKET m_socket = INVALID_SOCKET;

public:
	CSocket() noexcept = default;
	~CSocket() noexcept;

	CSocket(CSocket&& _other) noexcept;
	CSocket& operator=(CSocket&& _other) noexcept;

	bool	Initialize() noexcept;
	bool	Close() noexcept;

	bool	Bind(const CIpEndPoint& _endPoint) noexcept;
	bool	Listen(const int _backlog = SOMAXCONN) noexcept;
	bool	Connect(const CIpEndPoint& _endPoint) noexcept;

	int		Send(const char* _packet, const int _size, const int _flags = 0) noexcept;
	int		Recv(char* _buffer, const int _size, const int _flags = 0) noexcept;

	bool	PostRecv(WSABUF _buffer, WSAOVERLAPPED& _overlapped) noexcept;
	bool	PostSend(WSABUF _buffer, WSAOVERLAPPED& _overlapped) noexcept;
	bool	PostAccept(const CSocket& _socket, byte* _addrBuf, const DWORD _addrBufSize, WSAOVERLAPPED& _overlapped) noexcept;
	bool	ApplyAcceptContext(const CSocket& _listenSocket) noexcept;

	bool	IsValid() const noexcept;
	SOCKET	GetSocket() const noexcept;
};
