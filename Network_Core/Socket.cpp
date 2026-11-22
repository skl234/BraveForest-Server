#include "Socket.h"
#include <MSWSock.h>

CSocket::~CSocket() noexcept
{
	Close();
}

CSocket::CSocket(CSocket&& _other) noexcept:
	m_socket(_other.m_socket)
{
	_other.m_socket = INVALID_SOCKET;
}

CSocket& CSocket::operator=(CSocket&& _other) noexcept
{
	if (this == &_other)
		return *this;

	CSocket socket;
	socket.m_socket = m_socket;
	m_socket = _other.m_socket;
	_other.m_socket = INVALID_SOCKET;

	return *this;
}

bool CSocket::Initialize() noexcept
{
	if (IsValid()) return false;

	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	return IsValid();
}

bool CSocket::Close() noexcept
{
	if (!IsValid()) return true;
	if (closesocket(m_socket) == SOCKET_ERROR) return false;

	m_socket = INVALID_SOCKET;
	return true;
}

bool CSocket::Bind(const CIpEndPoint& _endPoint) noexcept
{
	const SOCKADDR_IN& addr = _endPoint.GetAddress();
	return bind(m_socket, reinterpret_cast<const SOCKADDR*>(&addr), sizeof(addr)) != SOCKET_ERROR;
}

bool CSocket::Listen(const int _backlog) noexcept
{
	return listen(m_socket, _backlog) != SOCKET_ERROR;
}

bool CSocket::Connect(const CIpEndPoint& _endPoint) noexcept
{
	const SOCKADDR_IN& addr = _endPoint.GetAddress();
	return connect(m_socket, reinterpret_cast<const SOCKADDR*>(&addr), sizeof(addr)) != SOCKET_ERROR;
}

int CSocket::Send(const char* _packet, const int _size, const int _flags) noexcept
{
	return send(m_socket, _packet, _size, _flags);
}

int CSocket::Recv(char* _buffer, const int _size, const int _flags) noexcept
{
	return recv(m_socket, _buffer, _size, _flags);
}

bool CSocket::PostRecv(WSABUF _buffer, WSAOVERLAPPED& _overlapped) noexcept
{
	if (_buffer.buf == nullptr || _buffer.len == 0)
		return false;

	if (!IsValid())
		return false;

	DWORD flags = 0;
	if (WSARecv(m_socket, &_buffer, 1, nullptr, &flags, &_overlapped, nullptr) != SOCKET_ERROR)
		return true;

	return WSAGetLastError() == WSA_IO_PENDING;
}

bool CSocket::PostSend(WSABUF _buffer, WSAOVERLAPPED& _overlapped) noexcept
{
	if (_buffer.buf == nullptr || _buffer.len == 0)
		return false;

	if (!IsValid())
		return false;

	if (WSASend(m_socket, &_buffer, 1, nullptr, 0, &_overlapped, nullptr) != SOCKET_ERROR)
		return true;

	return WSAGetLastError() == WSA_IO_PENDING;
}

bool CSocket::PostAccept(const CSocket& _socket, byte* _addrBuf, const DWORD _addrBufSize, WSAOVERLAPPED& _overlapped) noexcept
{
	const DWORD addrSize = sizeof(SOCKADDR_IN) + 16;
	if (!_socket.IsValid() || _addrBuf == nullptr || _addrBufSize < addrSize * 2 || _overlapped.hEvent != nullptr)
		return false;

	if (!IsValid())
		return false;

	DWORD byteTrans = 0;
	if (AcceptEx(m_socket, _socket.GetSocket(), _addrBuf, 0, addrSize, addrSize, &byteTrans, &_overlapped))
		return true;

	return WSAGetLastError() == WSA_IO_PENDING;
}

bool CSocket::ApplyAcceptContext(const CSocket& _listenSocket) noexcept
{
	if (!_listenSocket.IsValid())
		return false;

	const SOCKET listenSocket = _listenSocket.GetSocket();

	return setsockopt(m_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		reinterpret_cast<const char*>(&listenSocket), sizeof(listenSocket)) != SOCKET_ERROR;
}

bool CSocket::IsValid() const noexcept
{
	return m_socket != INVALID_SOCKET;
}

SOCKET CSocket::GetSocket() const noexcept
{
	return m_socket;
}
