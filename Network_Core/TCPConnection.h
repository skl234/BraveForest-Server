#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include "IOCP.h"
#include "OverlappedManager.h"
#include "../Utility_Core/RingBuffer.h"
#include "../Utility_Core/ThreadSafeQueue.h"

enum class eConnectionState : uint16_t
{
	Disconnected = 0,	//연결x
	Connected,			//연결o
	Disconnecting,		//종료패킷을 받긴 했지만 그에 따른 정리상태
						//완전히 연결x인건 아님
						//추가적인 send와 recv는 하지 않는 그런 상태
};

class CTCPConnection
{
protected:
	SOCKET							m_socket = INVALID_SOCKET;
	SOCKADDR_IN						m_remoteAddr = {};
	std::atomic<eConnectionState>	m_state = eConnectionState::Disconnected;
	std::atomic<uint64_t>			m_IOpendingCount = 0;	//걸어놓은 IO갯수
	std::atomic<uint64_t>			m_generation = 0;		//logic에서의 소유자 확인용

	COverlappedManager*				m_overlappedManager = nullptr;
	CRingBuffer						m_recvBuffer;
	std::atomic<bool>				m_sendPending = false;
	CMPMCQueue<std::shared_ptr<std::vector<byte>>> m_sendBufferQueue;
	
	std::function<void()>			m_disconnectHandler;
	std::function<void()>			m_recvHandler;
	std::function<void(DWORD)>		m_errorHandler;

public:
	CTCPConnection(COverlappedManager* _overlappedManager, uint64_t _recvBufferSize);
	virtual ~CTCPConnection() = default;

	bool Connect(SOCKET _socket, SOCKADDR_IN& _addr, CIOCP* _iocp);
	bool Connect(const std::string& _addr, uint16_t _port, CIOCP* _iocp);
	void Disconnect();
	bool IsConnected();
	uint64_t GetGeneration();

	int  Send(char* _packet, int _size);
	bool PostRecv();
	bool PostSend(std::shared_ptr<std::vector<byte>> _packet);

	void HandleRecv(WSAOVERLAPPED_EX* _overlapped, DWORD _byteTrans);
	void HandleSend(WSAOVERLAPPED_EX* _overlapped, DWORD _byteTrans);
	void HandleError(WSAOVERLAPPED_EX* _overlapped, DWORD _error);

	CRingBuffer& GetRecvBuffer();
	
private:
	bool TrySend(); //PostSend
	void OnCompleteIO();
	void TryDisconnect();

public:
	void SetDisconnectHandler(std::function<void()> _callback);
	void SetRecvHandler(std::function<void()> _callback);
	void SetErrorHandler(std::function<void(DWORD)> _callback);
};
