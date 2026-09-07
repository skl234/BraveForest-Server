#include "TCPAcceptor.h"
#include <WS2tcpip.h>
#include <iostream>
#include <mswsock.h>
#include "OverlappedManager.h"
#include "IOCP.h"
#include "../Utility_Core/LogManager.h"

CTCPAcceptor::CTCPAcceptor(const std::string& _addr, uint16_t _port)
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_addr = { 0, };
	inet_pton(AF_INET, _addr.data(), &m_addr.sin_addr);
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(_port);
}

CTCPAcceptor::~CTCPAcceptor()
{
	Disconnect();
}

bool CTCPAcceptor::Initialize(const std::string& _addr, uint16_t _port)
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) return false;

	m_addr = { 0, };
	inet_pton(AF_INET, _addr.data(), &m_addr.sin_addr);
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(_port);

	return true;
}

bool CTCPAcceptor::Start(uint64_t _postSize, CIOCP* _iocp, COverlappedManager* _overlappedManager)
{
	if (!m_createConnectionHandler) return false;
	if (!m_errorHandler) return false;

	m_iocp = _iocp;
	m_overlappedManager = _overlappedManager;

	BOOL reuse = TRUE;
	if (SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)))
		return false;

	if (SOCKET_ERROR == bind(m_socket, reinterpret_cast<SOCKADDR*>(&m_addr), sizeof(m_addr)))
		return false;

	if (SOCKET_ERROR == listen(m_socket, SOMAXCONN))
		//SOMAX 갯수보다 
		return false;

	if (!m_iocp->Regist(reinterpret_cast<HANDLE>(m_socket), reinterpret_cast<ULONG_PTR>(this)))
		return false;

	for (uint64_t i = 0; i < _postSize; ++i)
	{
		if (!Accept()) return false;
	}

	return true;
}

void CTCPAcceptor::Disconnect()
{
	if (INVALID_SOCKET != m_socket) closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

void CTCPAcceptor::HandleAccept(WSAOVERLAPPED_EX* _overlapped, DWORD _byteTrans)
{
	if (SOCKET_ERROR == setsockopt(_overlapped->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_socket, sizeof(m_socket)))
	{
		CLogManager::GetInstance().Write(eLogLevel::eERROR,
			"TCPAcceptor Handle(), setsockopt Error : %d", WSAGetLastError());

		closesocket(_overlapped->socket);
		m_overlappedManager->Release(_overlapped);

		Accept();

		return;
	}

	SOCKET		 remoteSocket = _overlapped->socket;
	SOCKADDR_IN* remoteAddr = nullptr;
	SOCKADDR_IN* localAddr = nullptr;
	int	remoteAddrLen = 0;
	int localAddrLen = 0;
	GetAcceptExSockaddrs(_overlapped->addrBuf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		(SOCKADDR**)&localAddr, &localAddrLen, (SOCKADDR**)&remoteAddr, &remoteAddrLen);

	m_overlappedManager->Release(_overlapped);

	m_createConnectionHandler(remoteSocket, *remoteAddr, m_iocp);

	Accept();
}

void CTCPAcceptor::HandleError(WSAOVERLAPPED_EX* _overlapped, DWORD _error)
{
	closesocket(_overlapped->socket);
	m_overlappedManager->Release(_overlapped);

	switch (_error)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
	case ERROR_NETNAME_DELETED:
		Accept();
		break;

	default:
		m_errorHandler(_error);
		break;
	}
}

SOCKET CTCPAcceptor::GetSocket()
{
	return m_socket;
}

void CTCPAcceptor::SetCreateConnectionHandler(std::function<void(SOCKET, SOCKADDR_IN, CIOCP*)> _callback)
{
	m_createConnectionHandler = _callback;
}

void CTCPAcceptor::SetErrorHandler(std::function<void(DWORD)> _callback)
{
	m_errorHandler = _callback;
}

bool CTCPAcceptor::Accept()
{
	WSAOVERLAPPED_EX* overlapped = m_overlappedManager->Acquire();
	if (!overlapped) return false;

	overlapped->type = eOverlappedType::Accept;
	overlapped->owner = this;
	overlapped->socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	
	if (overlapped->socket == INVALID_SOCKET)
	{
		m_overlappedManager->Release(overlapped);
		return false;
	}

	if (!AcceptEx(m_socket, overlapped->socket,
		overlapped->addrBuf, 0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		NULL,
		static_cast<WSAOVERLAPPED*>(overlapped)))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			CLogManager::GetInstance().Write(eLogLevel::eERROR,
				"TCPAcceptor PostAccept() AcceptEx fail : %d", WSAGetLastError());
			
			closesocket(overlapped->socket);
			m_overlappedManager->Release(overlapped);

			return false;
		}
	}

	return true;
}
