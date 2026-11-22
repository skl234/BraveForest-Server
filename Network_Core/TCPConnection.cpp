#include "TCPConnection.h"
#include <cassert>
#include <utility>
#include "../Utility_Core/LockGuard.h"

CTCPConnection::CTCPConnection(CIOCPEventManager* _eventManager, uint64_t _recvBufferSize):
	m_eventManager(_eventManager),
	m_recvBuffer(_recvBufferSize)
{
	assert(m_eventManager != nullptr);
	assert(_recvBufferSize > 1);

	InitializeSRWLock(&m_stateLock);
}

CTCPConnection::~CTCPConnection()
{
	assert(m_IOpendingCount.load(std::memory_order_acquire) == 0);
}

bool CTCPConnection::Connect(CSocket _socket, const CIpEndPoint& _remoteEndPoint, const CIOCP& _iocp)
{
	if (m_eventManager == nullptr || _remoteEndPoint.GetAddress().sin_family != AF_INET)
		return false;

	if (!_socket.IsValid() || !_iocp.IsInit())
		return false;

	{
		CLockGuard lockGuard(m_stateLock);

		if (m_state.load(std::memory_order_acquire) != eConnectionState::Disconnected)
			return false;
		if (m_socket.IsValid() || m_IOpendingCount.load(std::memory_order_acquire) != 0)
			return false;
		if (!_iocp.Regist(reinterpret_cast<HANDLE>(_socket.GetSocket()), reinterpret_cast<ULONG_PTR>(this)))
			return false;

		m_socket = std::move(_socket);
		m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);

		m_remoteEndPoint = _remoteEndPoint;
		m_generation.fetch_add(1, std::memory_order_relaxed);
		m_state.store(eConnectionState::Connected, std::memory_order_release);
	}

	bool result = PostRecv();
	if (!result) Disconnect();
	OnCompleteIO();

	return result;
}

bool CTCPConnection::Connect(const CIpEndPoint& _remoteEndPoint, const CIOCP& _iocp)
{
	if (m_eventManager == nullptr || !_iocp.IsInit())
		return false;
	if (m_state.load(std::memory_order_acquire) != eConnectionState::Disconnected)
		return false;

	CSocket socket;
	if (!socket.Initialize() || !socket.Connect(_remoteEndPoint))
		return false;

	return Connect(std::move(socket), _remoteEndPoint, _iocp);
}

bool CTCPConnection::Disconnect()
{
	bool result = true;
	{
		CLockGuard lockGuard(m_stateLock);
		if (m_state.load(std::memory_order_acquire) == eConnectionState::Disconnected)
			return true;
		if (m_state.load(std::memory_order_acquire) == eConnectionState::Disconnecting && !m_socket.IsValid())
			return true;
		m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);
		m_state.store(eConnectionState::Disconnecting, std::memory_order_release);

		if (m_socket.IsValid())
			result = m_socket.Close();
	}

	m_sendBufferQueue.Clear();
	OnCompleteIO();

	return result;
}

bool CTCPConnection::IsConnected()
{
	return m_state.load(std::memory_order_acquire) == eConnectionState::Connected;
}

bool CTCPConnection::CanDestroy() noexcept
{
	CLockGuard lockGuard(m_stateLock, eLockMode::Shared);

	return m_state.load(std::memory_order_acquire) == eConnectionState::Disconnected &&
		!m_socket.IsValid() && m_IOpendingCount.load(std::memory_order_acquire) == 0;
}

bool CTCPConnection::PostSend(std::shared_ptr<std::vector<byte>> _packet)
{
	if (_packet == nullptr || _packet->empty())
		return false;
	{
		CLockGuard lockGuard(m_stateLock);

		if (!IsConnected())
			return false;

		m_sendBufferQueue.Push(std::move(_packet));
		m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);
	}

	const bool result = TrySend();
	OnCompleteIO();

	return result;
}

void CTCPConnection::HandleRecv(CRecvEvent& _event, DWORD _byteTrans)
{
	m_eventManager->ReleaseRecv(&_event);

	if (_byteTrans == 0)
	{
		Disconnect();
		return;
	}
	if (!IsConnected())
		return;
	if (!m_recvBuffer.MoveWritePointer(static_cast<uint64_t>(_byteTrans)))
	{
		Disconnect();
		return;
	}

	if (m_recvHandler) m_recvHandler();
	if (IsConnected() && !PostRecv()) Disconnect();
}

void CTCPConnection::HandleSend(CSendEvent& _event)
{
	m_eventManager->ReleaseSend(&_event);
	m_sendPending.store(false, std::memory_order_release);

	if (IsConnected()) TrySend();
}

void CTCPConnection::HandleRecvError(CRecvEvent& _event, DWORD _error)
{
	m_eventManager->ReleaseRecv(&_event);
	Disconnect();

	if (m_errorHandler) m_errorHandler(_error);
}

void CTCPConnection::HandleSendError(CSendEvent& _event, DWORD _error)
{
	m_eventManager->ReleaseSend(&_event);
	Disconnect();
	m_sendPending.store(false, std::memory_order_release);

	if (m_errorHandler) m_errorHandler(_error);
}

void CTCPConnection::OnCompleteIO()
{
	{
		CLockGuard lockGuard(m_stateLock);

		const uint64_t pending = m_IOpendingCount.load(std::memory_order_acquire);
		assert(pending != 0);
		if (pending == 0)
			return;

		if (pending != 1 || m_state.load(std::memory_order_acquire) != eConnectionState::Disconnecting || m_socket.IsValid())
		{
			m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);
			return;
		}

		m_recvBuffer.Cleanup();
		m_sendBufferQueue.Clear();
		m_remoteEndPoint = {};
		m_sendPending.store(false, std::memory_order_release);
	}

	if (m_disconnectHandler) m_disconnectHandler();

	CLockGuard lockGuard(m_stateLock);
	m_state.store(eConnectionState::Disconnected, std::memory_order_release);
	m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);
}

CRingBuffer& CTCPConnection::GetRecvBuffer()
{
	return m_recvBuffer;
}

uint64_t CTCPConnection::GetGeneration()
{
	return m_generation.load();
}

bool CTCPConnection::PostRecv()
{
	CLockGuard lockGuard(m_stateLock);

	if (!IsConnected())
		return false;

	uint64_t size = m_recvBuffer.GetDirectWriteableSize();
	if (size == 0 || size > UINT32_MAX)
		return false;

	CRecvEvent* event = m_eventManager->AcquireRecv();
	if (event == nullptr)
		return false;
	if (event->Initialize(this, m_recvBuffer.GetWritePointer(), static_cast<ULONG>(size)))
	{
		IOCP_CONTEXT& context = event->GetContext();
		m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);

		if (m_socket.PostRecv(event->GetBuffer(), context.overlapped))
			return true;

		m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);
	}

	m_eventManager->ReleaseRecv(event);
	return false;
}

bool CTCPConnection::TrySend()
{
	CSendEvent* event = nullptr;
	{
		CLockGuard lockGuard(m_stateLock);
		if (!IsConnected())
			return false;

		bool expected = false;
		if (!m_sendPending.compare_exchange_strong(expected, true))
			return true;

		std::shared_ptr<std::vector<byte>> packet;
		if (!m_sendBufferQueue.Pop(packet))
		{
			m_sendPending.store(false, std::memory_order_release);
			return true;
		}

		event = m_eventManager->AcquireSend();
		if (event != nullptr)
		{
			if (event->Initialize(this, std::move(packet)))
			{
				IOCP_CONTEXT& context = event->GetContext();
				m_IOpendingCount.fetch_add(1, std::memory_order_relaxed);

				if (m_socket.PostSend(event->GetBuffer(), context.overlapped))
					return true;

				m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);
			}
		}
	}

	Disconnect();
	m_sendPending.store(false, std::memory_order_release);
	if (event != nullptr) m_eventManager->ReleaseSend(event);
	return false;
}

void CTCPConnection::SetDisconnectHandler(std::function<void()> _callback)
{
	m_disconnectHandler = std::move(_callback);
}

void CTCPConnection::SetRecvHandler(std::function<void()> _callback)
{
	m_recvHandler = std::move(_callback);
}

void CTCPConnection::SetErrorHandler(std::function<void(DWORD)> _callback)
{
	m_errorHandler = std::move(_callback);
}
