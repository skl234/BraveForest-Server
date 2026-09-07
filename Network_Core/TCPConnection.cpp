#include "TCPConnection.h"
#include "IOCP.h"
#include "OverlappedManager.h"
#include "../Utility_Core/LogManager.h"
#include "../Common/PACKET.h"
#include <cassert>

CTCPConnection::CTCPConnection(COverlappedManager* _overlappedManager, uint64_t _recvBufferSize):
	m_overlappedManager(_overlappedManager),
	m_recvBuffer(_recvBufferSize)
{
}

bool CTCPConnection::Connect(SOCKET _socket, SOCKADDR_IN& _addr, CIOCP* _iocp)
{
	eConnectionState expected = eConnectionState::Disconnected;
	if (!m_state.compare_exchange_strong(expected, eConnectionState::Connected)) return false;
	m_generation.fetch_add(1, std::memory_order::memory_order_relaxed);

	m_socket = _socket;
	m_remoteAddr = _addr;

	if (!_iocp->Regist(reinterpret_cast<HANDLE>(m_socket), reinterpret_cast<ULONG_PTR>(this)))
	{
		Disconnect();
		return false;
	}

	if (!PostRecv())
	{
		Disconnect();
		return false;
	}

	return true;
}

bool CTCPConnection::Connect(const std::string& _addr, uint16_t _port, CIOCP* _iocp)
{
	eConnectionState expected = eConnectionState::Disconnected;
	if (!m_state.compare_exchange_strong(expected, eConnectionState::Connected)) return false;
	m_generation.fetch_add(1, std::memory_order::memory_order_relaxed);

	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET)
	{
		Disconnect();
		return false;
	}

	m_remoteAddr = {};
	m_remoteAddr.sin_family = AF_INET;
	m_remoteAddr.sin_port = htons(_port);
	inet_pton(AF_INET, _addr.c_str(), &m_remoteAddr.sin_addr);

	if (SOCKET_ERROR == connect(m_socket, reinterpret_cast<sockaddr*>(&m_remoteAddr), sizeof(m_remoteAddr)))
	{
		Disconnect();
		return false;
	}

	if (!_iocp->Regist(reinterpret_cast<HANDLE>(m_socket), reinterpret_cast<ULONG_PTR>(this)))
	{
		Disconnect();
		return false;
	}

	if (!PostRecv())
	{
		Disconnect();
		return false;
	}

	return true;
}

void CTCPConnection::Disconnect()
{
	eConnectionState expected = eConnectionState::Connected;
	if (!m_state.compare_exchange_strong(expected, eConnectionState::Disconnecting, std::memory_order_acq_rel))
	{
		return;
	}
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

	// 아직 WSASend가 발행되지 않은 패킷
	m_sendBufferQueue.Clear();

	TryDisconnect();
}

bool CTCPConnection::IsConnected()
{
	if (m_state.load() == eConnectionState::Connected) return true;
	return false;
}

uint64_t CTCPConnection::GetGeneration()
{
	return m_generation.load();
}

int CTCPConnection::Send(char* _packet, int _size)
{
	return send(m_socket, _packet, _size, 0);
}

bool CTCPConnection::PostRecv()
{
	if (m_state.load(std::memory_order_acquire) != eConnectionState::Connected) return false;

	WSABUF wsabuf;
	wsabuf.buf = reinterpret_cast<char*>(m_recvBuffer.GetWritePointer());
	wsabuf.len = static_cast<ULONG>(m_recvBuffer.GetDirectWriteableSize());
	if (wsabuf.len == 0) return false;

	WSAOVERLAPPED_EX* overlapped = m_overlappedManager->Acquire();
	if (overlapped == nullptr) return false;

	overlapped->type = eOverlappedType::Recv;
	overlapped->owner = this;

	m_IOpendingCount.fetch_add(1, std::memory_order_acq_rel);
	DWORD flags = 0;
	if (SOCKET_ERROR == WSARecv(m_socket, &wsabuf, 1, NULL, &flags, static_cast<WSAOVERLAPPED*>(overlapped), nullptr))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			m_overlappedManager->Release(overlapped);
			OnCompleteIO();
			return false;
		}
	}

	return true;
}

bool CTCPConnection::PostSend(std::shared_ptr<std::vector<byte>> _packet)
{
	if (m_socket == INVALID_SOCKET) return false;
	if (_packet == nullptr) return false;
	if (_packet->size() == 0) return false;
	if (!IsConnected()) return false;

	m_sendBufferQueue.Push(_packet);

	return TrySend();
}

void CTCPConnection::HandleRecv(WSAOVERLAPPED_EX* _overlapped, DWORD _byteTrans)
{
	m_overlappedManager->Release(_overlapped);

	if (_byteTrans == 0)
	{
		Disconnect();
		OnCompleteIO();
		return;
	}

	if (m_state.load(std::memory_order_acquire) != eConnectionState::Connected)
	{
		OnCompleteIO();
		return;
	}

	if (!m_recvBuffer.MoveWritePointer(static_cast<uint64_t>(_byteTrans)))
	{
		Disconnect();
		OnCompleteIO();
		return;
	}

	if (m_recvHandler) m_recvHandler();

	if (IsConnected() && !PostRecv()) Disconnect();

	OnCompleteIO();
}

void CTCPConnection::HandleSend(WSAOVERLAPPED_EX* _overlapped, DWORD _byteTrans)
{
	m_overlappedManager->Release(_overlapped);
	m_sendPending.store(false, std::memory_order_release);
	if (m_state.load(std::memory_order_acquire) == eConnectionState::Connected)
	{
		TrySend();
	}

	OnCompleteIO();
}

void CTCPConnection::HandleError(WSAOVERLAPPED_EX* _overlapped, DWORD _error)
{
	const eOverlappedType type = _overlapped->type;
	if (type == eOverlappedType::Send)
	{
		m_sendPending.store(false, std::memory_order_release);
	}

	m_overlappedManager->Release(_overlapped);

	if (m_errorHandler)m_errorHandler(_error);

	Disconnect();
	OnCompleteIO();
}

CRingBuffer& CTCPConnection::GetRecvBuffer()
{
	return m_recvBuffer;
}

bool CTCPConnection::TrySend()
{
	bool expected = false;
	if (!m_sendPending.compare_exchange_strong(expected, true)) return true;

	std::shared_ptr<std::vector<byte>> packet;
	if (!m_sendBufferQueue.Pop(packet))
	{
		m_sendPending.store(false, std::memory_order_release);
		/*
		 * Queue가 비었다고 확인한 직후,
		 * m_sendPending을 false로 바꾸기 전에
		 * 다른 Thread가 패킷을 Push했을 수 있다.
		 */
		if (!m_sendBufferQueue.IsEmpty())
		{
			return TrySend();
		}

		return true;
	}

	WSAOVERLAPPED_EX* overlapped = m_overlappedManager->Acquire();
	if (overlapped == nullptr)
	{
		m_sendPending.store(false, std::memory_order_release);
		/*
		 * 이미 Queue에서 packet을 꺼냈으므로
		 * Overlapped를 구하지 못하면 복구가 어렵다.
		 * 현재 구조에서는 연결을 끊는 것이 안전하다.
		 */
		Disconnect();
		return false;
	}

	overlapped->type = eOverlappedType::Send;
	overlapped->owner = this;
	overlapped->sendPacket = packet;

	WSABUF wsabuf;
	wsabuf.buf = reinterpret_cast<char*>(packet->data());
	wsabuf.len = static_cast<ULONG>(packet->size());

	m_IOpendingCount.fetch_add(1, std::memory_order_acq_rel);

	if (SOCKET_ERROR == WSASend(m_socket, &wsabuf, 1, nullptr, 0, static_cast<WSAOVERLAPPED*>(overlapped), nullptr))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			m_sendPending.store(false, std::memory_order_release);
			m_overlappedManager->Release(overlapped);

			OnCompleteIO();
			Disconnect();

			return false;
		}
	}

	return true;
}

void CTCPConnection::OnCompleteIO()
{
	m_IOpendingCount.fetch_sub(1, std::memory_order_acq_rel);

	if (m_IOpendingCount.load() == 0) TryDisconnect();
}

void CTCPConnection::TryDisconnect()
{
	if (m_IOpendingCount.load(std::memory_order_acquire) != 0) return;

	eConnectionState expected = eConnectionState::Disconnecting;
	if (!m_state.compare_exchange_strong(expected, eConnectionState::Disconnected, std::memory_order_acq_rel))
	{
		return;
	}

	m_recvBuffer.Cleanup();
	m_sendBufferQueue.Clear();
	m_remoteAddr = {};
	m_sendPending.store(false, std::memory_order_release);

	if (m_disconnectHandler) m_disconnectHandler();
}

void CTCPConnection::SetDisconnectHandler(std::function<void()> _callback)
{
	m_disconnectHandler = _callback;
}

void CTCPConnection::SetRecvHandler(std::function<void()> _callback)
{
	m_recvHandler = _callback;
}

//void CTCPConnection::OnSend(std::function<void()> _callback)
//{
//	m_onSendListener = _callback;
//}

void CTCPConnection::SetErrorHandler(std::function<void(DWORD)> _callback)
{
	m_errorHandler = _callback;
}
