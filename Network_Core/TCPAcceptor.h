#pragma once
#include <WinSock2.h>
#include <string>
#include <functional>
#include "IOCP.h"
#include "OverlappedManager.h"
#include "../Utility_Core/UtilityMacros.h"

class CTCPAcceptor
{
protected:
	CIOCP*				m_iocp = nullptr;
	COverlappedManager* m_overlappedManager = nullptr;

	SOCKET				m_socket = INVALID_SOCKET;
	SOCKADDR_IN			m_addr = { 0, };

	std::function<void(SOCKET, SOCKADDR_IN, CIOCP*)> m_createConnectionHandler;
	std::function<void(DWORD)>						 m_errorHandler;

public:
	CTCPAcceptor() = default;
	CTCPAcceptor(const std::string& _addr, uint16_t _port);
	virtual ~CTCPAcceptor();

	bool Initialize(const std::string& _addr, uint16_t _port);
	bool Start(uint64_t _postSize, CIOCP* _iocp, COverlappedManager* _overlappedManager);
	void Disconnect();
	void HandleAccept(WSAOVERLAPPED_EX* _overlapped, DWORD _byteTrans);
	void HandleError(WSAOVERLAPPED_EX* _overlapped, DWORD _error);
	
	SOCKET GetSocket();

public:
	void SetCreateConnectionHandler(std::function<void(SOCKET, SOCKADDR_IN, CIOCP*)> _callback);
	void SetErrorHandler(std::function<void(DWORD)> _callback);

private:
	bool Accept();
};
