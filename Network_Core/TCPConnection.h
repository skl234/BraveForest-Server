#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include "Socket.h"
#include "IOCP.h"
#include "IOCPEventManager.h"
#include "../Utility_Core/RingBuffer.h"
#include "../Utility_Core/ThreadSafeQueue.h"

enum class eConnectionState : uint16_t
{
	Disconnected = 0,	//연결x
	Connected,			//연결o
	Disconnecting,		//남은 I/O 완료를 처리하는 상태
};

class CTCPConnection
{
protected:
	SRWLOCK								m_stateLock;

	CSocket								m_socket;
	CIpEndPoint							m_remoteEndPoint;
	std::atomic<eConnectionState>		m_state = eConnectionState::Disconnected;
	std::atomic<uint64_t>				m_IOpendingCount = 0;
	std::atomic<uint64_t>				m_generation = 0;

	CIOCPEventManager*					m_eventManager = nullptr;
	CRingBuffer							m_recvBuffer;
	std::atomic<bool>					m_sendPending = false;
	CMPMCQueue<std::shared_ptr<std::vector<byte>>> m_sendBufferQueue;

	std::function<void()>				m_disconnectHandler;
	std::function<void()>				m_recvHandler;
	std::function<void(DWORD)>			m_errorHandler;

public:
	CTCPConnection(CIOCPEventManager* _eventManager, uint64_t _recvBufferSize);
	virtual ~CTCPConnection();

	bool Connect(CSocket _socket, const CIpEndPoint& _remoteEndPoint, const CIOCP& _iocp);
	bool Connect(const CIpEndPoint& _remoteEndPoint, const CIOCP& _iocp);
	bool Disconnect();

	bool IsConnected();
	bool CanDestroy() noexcept;

	bool PostSend(std::shared_ptr<std::vector<byte>> _packet);

	void HandleRecv(CRecvEvent& _event, DWORD _byteTrans);
	void HandleSend(CSendEvent& _event);
	void HandleRecvError(CRecvEvent& _event, DWORD _error);
	void HandleSendError(CSendEvent& _event, DWORD _error);
	void OnCompleteIO();

	CRingBuffer& GetRecvBuffer();
	uint64_t GetGeneration();

private:
	bool PostRecv();
	bool TrySend();

public:
	void SetDisconnectHandler(std::function<void()> _callback);
	void SetRecvHandler(std::function<void()> _callback);
	void SetErrorHandler(std::function<void(DWORD)> _callback);
};
