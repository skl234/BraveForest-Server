#pragma once
#include <functional>
#include <atomic>
#include "Socket.h"
#include "IOCP.h"
#include "IOCPEventManager.h"

class CTCPAcceptor
{
protected:
	SRWLOCK								m_stateLock;

	CSocket								m_socket;
	CIpEndPoint							m_localEndPoint;
	std::atomic<uint64_t>				m_IOpendingCount = 0;

	const CIOCP*						m_iocp = nullptr;
	CIOCPEventManager*					m_eventManager = nullptr;

	std::function<void(CSocket, const CIpEndPoint&, const CIOCP&)> m_createConnectionHandler;
	std::function<void(DWORD)>			m_errorHandler;

public:
	CTCPAcceptor();
	explicit CTCPAcceptor(const CIpEndPoint& _localEndPoint);
	virtual ~CTCPAcceptor();

	bool Initialize(const CIpEndPoint& _localEndPoint);
	bool Start(uint64_t _postSize, const CIOCP& _iocp, CIOCPEventManager* _eventManager);
	bool Disconnect();
	bool CanDestroy() noexcept;

	void HandleAccept(CAcceptEvent& _event);
	void HandleError(CAcceptEvent& _event, DWORD _error);
	void OnCompleteIO();

public:
	void SetCreateConnectionHandler(std::function<void(CSocket, const CIpEndPoint&, const CIOCP&)> _callback);
	void SetErrorHandler(std::function<void(DWORD)> _callback);

private:
	bool Accept();
};
