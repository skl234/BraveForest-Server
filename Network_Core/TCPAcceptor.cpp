#include "TCPAcceptor.h"
#include <cassert>
#include <utility>
#include "../Utility_Core/LockGuard.h"

CTCPAcceptor::CTCPAcceptor()
{
	InitializeSRWLock(&m_stateLock);
}

CTCPAcceptor::CTCPAcceptor(const CIpEndPoint& _localEndPoint)
{
	InitializeSRWLock(&m_stateLock);
	Initialize(_localEndPoint);
}

CTCPAcceptor::~CTCPAcceptor()
{
	Disconnect();
	assert(m_IOpendingCount.load(std::memory_order_acquire) == 0);
}

bool CTCPAcceptor::Initialize(const CIpEndPoint& _localEndPoint)
{
	CLockGuard lockGuard(m_stateLock);

	if (m_socket.IsValid() || m_IOpendingCount.load(std::memory_order_acquire) != 0)
		return false;

	if (_localEndPoint.GetAddress().sin_family != AF_INET)
		return false;

	m_iocp = nullptr;
	m_eventManager = nullptr;
	m_localEndPoint = _localEndPoint;

	return true;
}

bool CTCPAcceptor::Start(uint64_t _postSize, const CIOCP& _iocp, CIOCPEventManager* _eventManager)
{
	if (_postSize == 0 || _eventManager == nullptr || !_iocp.IsInit())
		return false;

	{
		CLockGuard lockGuard(m_stateLock);

		if (m_iocp != nullptr || m_localEndPoint.GetAddress().sin_family != AF_INET)
			return false;
		if (m_socket.IsValid() || m_IOpendingCount.load(std::memory_order_acquire) != 0)
			return false;
		if (!m_createConnectionHandler || !m_errorHandler)
			return false;

		CSocket socket;
		if (!socket.Initialize())
			return false;
		if (!socket.Bind(m_localEndPoint) || !socket.Listen())
			return false;
		if (!_iocp.Regist(reinterpret_cast<HANDLE>(socket.GetSocket()), reinterpret_cast<ULONG_PTR>(this)))
			return false;

		m_socket = std::move(socket);

		m_iocp = &_iocp;
		m_eventManager = _eventManager;
		m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);
	}

	for (uint64_t i = 0; i < _postSize; ++i)
	{
		if (!Accept())
		{
			Disconnect();
			OnCompleteIO();
			return false;
		}
	}

	OnCompleteIO();
	return true;
}

bool CTCPAcceptor::Disconnect()
{
	CLockGuard lockGuard(m_stateLock);

	if (!m_socket.IsValid())
		return true;

	return m_socket.Close();
}

bool CTCPAcceptor::CanDestroy() noexcept
{
	CLockGuard lockGuard(m_stateLock, eLockMode::Shared);

	return !m_socket.IsValid() && m_IOpendingCount.load(std::memory_order_acquire) == 0;
}

void CTCPAcceptor::HandleAccept(CAcceptEvent& _event)
{
	bool result = false;
	{
		CLockGuard lockGuard(m_stateLock, eLockMode::Shared);
		result = _event.GetSocket().ApplyAcceptContext(m_socket);
	}
	if (!result)
	{
		m_eventManager->ReleaseAccept(&_event);
		Disconnect();
		return;
	}
	CIpEndPoint remoteEndPoint;
	if (!_event.GetRemoteEndPoint(remoteEndPoint))
	{
		HandleError(_event, WSAEFAULT);
		return;
	}

	CSocket socket = _event.TakeSocket();
	m_eventManager->ReleaseAccept(&_event);
	m_createConnectionHandler(std::move(socket), remoteEndPoint, *m_iocp);

	if (!Accept()) Disconnect();
}

void CTCPAcceptor::HandleError(CAcceptEvent& _event, DWORD _error)
{
	m_eventManager->ReleaseAccept(&_event);
	{
		CLockGuard lockGuard(m_stateLock, eLockMode::Shared);
		if (!m_socket.IsValid())
			return;
	}
	switch (_error)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
	case ERROR_NETNAME_DELETED:
		if (!Accept()) Disconnect();
		break;
	default:
		m_errorHandler(_error);
		break;
	}
}

void CTCPAcceptor::OnCompleteIO()
{
	CLockGuard lockGuard(m_stateLock);

	const uint64_t pending = m_IOpendingCount.load(std::memory_order_acquire);
	assert(pending != 0);

	if (pending == 0)
		return;

	m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);
}

void CTCPAcceptor::SetCreateConnectionHandler(std::function<void(CSocket, const CIpEndPoint&, const CIOCP&)> _callback)
{
	m_createConnectionHandler = std::move(_callback);
}

void CTCPAcceptor::SetErrorHandler(std::function<void(DWORD)> _callback)
{
	m_errorHandler = std::move(_callback);
}

bool CTCPAcceptor::Accept()
{
	CLockGuard lockGuard(m_stateLock);

	if (!m_socket.IsValid())
		return false;

	CAcceptEvent* event = m_eventManager->AcquireAccept();
	if (event == nullptr)
		return false;

	if (event->Initialize(this))
	{
		IOCP_CONTEXT& context = event->GetContext();
		m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);

		if (m_socket.PostAccept(event->GetSocket(), event->GetAddressBuffer(), event->GetAddressBufferSize(),context.overlapped))
			return true;

		m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);
	}

	m_eventManager->ReleaseAccept(event);
	return false;
}
