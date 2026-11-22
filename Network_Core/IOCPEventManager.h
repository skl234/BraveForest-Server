#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <deque>
#include "RecvEvent.h"
#include "SendEvent.h"
#include "AcceptEvent.h"

class CIOCPEventManager
{
public:
	DELETE_COPY_MOVE(CIOCPEventManager);

private:
	bool m_isInit = false;

	std::vector<std::unique_ptr<CRecvEvent>>	m_recvEventList;
	std::deque<CRecvEvent*>						m_recvEventPool;
	SRWLOCK										m_recvEventLock;

	std::vector<std::unique_ptr<CSendEvent>>	m_sendEventList;
	std::deque<CSendEvent*>						m_sendEventPool;
	SRWLOCK										m_sendEventLock;

	std::vector<std::unique_ptr<CAcceptEvent>>	m_acceptEventList;
	std::deque<CAcceptEvent*>					m_acceptEventPool;
	SRWLOCK										m_acceptEventLock;

public:
	CIOCPEventManager();
	~CIOCPEventManager() = default;

	bool		Initialize(const uint64_t _recvSize, const uint64_t _sendSize, const uint64_t _acceptSize);
	// 대여하는 스레드를 종료한 뒤 호출한다.
	bool		Cleanup();
	uint64_t	GetAcquireSize();

	CRecvEvent*		AcquireRecv();
	CSendEvent*		AcquireSend();
	CAcceptEvent*	AcquireAccept();

	void ReleaseRecv(CRecvEvent* _event);
	void ReleaseSend(CSendEvent* _event);
	void ReleaseAccept(CAcceptEvent* _event);
};
