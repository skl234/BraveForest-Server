#include "IOCPEventManager.h"
#include <cassert>
#include <utility>
#include "../Utility_Core/LockGuard.h"

CIOCPEventManager::CIOCPEventManager()
{
	InitializeSRWLock(&m_recvEventLock);
	InitializeSRWLock(&m_sendEventLock);
	InitializeSRWLock(&m_acceptEventLock);
}

bool CIOCPEventManager::Initialize(const uint64_t _recvSize, const uint64_t _sendSize, const uint64_t _acceptSize)
{
	if (m_isInit)
	{
		SetLastError(ERROR_ALREADY_INITIALIZED);
		return false;
	}
	if (_recvSize > SIZE_MAX || _sendSize > SIZE_MAX || _acceptSize > SIZE_MAX)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}

	m_recvEventList.reserve(static_cast<size_t>(_recvSize));
	for (uint64_t i = 0; i < _recvSize; ++i)
	{
		std::unique_ptr<CRecvEvent> event = std::make_unique<CRecvEvent>();
		m_recvEventPool.push_back(event.get());
		m_recvEventList.emplace_back(std::move(event));
	}

	m_sendEventList.reserve(static_cast<size_t>(_sendSize));
	for (uint64_t i = 0; i < _sendSize; ++i)
	{
		std::unique_ptr<CSendEvent> event = std::make_unique<CSendEvent>();
		m_sendEventPool.push_back(event.get());
		m_sendEventList.emplace_back(std::move(event));
	}

	m_acceptEventList.reserve(static_cast<size_t>(_acceptSize));
	for (uint64_t i = 0; i < _acceptSize; ++i)
	{
		std::unique_ptr<CAcceptEvent> event = std::make_unique<CAcceptEvent>();
		m_acceptEventPool.push_back(event.get());
		m_acceptEventList.emplace_back(std::move(event));
	}

	m_isInit = true;
	return true;
}

bool CIOCPEventManager::Cleanup()
{
	if (GetAcquireSize() != 0) return false;

	m_recvEventPool.clear();
	m_sendEventPool.clear();
	m_acceptEventPool.clear();
	m_recvEventList.clear();
	m_sendEventList.clear();
	m_acceptEventList.clear();
	m_isInit = false;
	return true;
}

uint64_t CIOCPEventManager::GetAcquireSize()
{
	CLockGuard recvLock(m_recvEventLock, eLockMode::Shared);
	CLockGuard sendLock(m_sendEventLock, eLockMode::Shared);
	CLockGuard acceptLock(m_acceptEventLock, eLockMode::Shared);

	assert(m_recvEventPool.size() <= m_recvEventList.size());
	assert(m_sendEventPool.size() <= m_sendEventList.size());
	assert(m_acceptEventPool.size() <= m_acceptEventList.size());
	uint64_t size = m_recvEventList.size() - m_recvEventPool.size();
	size += m_sendEventList.size() - m_sendEventPool.size();
	size += m_acceptEventList.size() - m_acceptEventPool.size();
	return size;
}

CRecvEvent* CIOCPEventManager::AcquireRecv()
{
	CLockGuard lockGuard(m_recvEventLock);
	if (m_recvEventPool.empty()) return nullptr;

	CRecvEvent* event = m_recvEventPool.front();
	m_recvEventPool.pop_front();
	return event;
}

void CIOCPEventManager::ReleaseRecv(CRecvEvent* _event)
{
	if (_event == nullptr) return;
	_event->GetContext().overlapped = {};
	_event->Cleanup();

	CLockGuard lockGuard(m_recvEventLock);
	m_recvEventPool.push_back(_event);
}

CSendEvent* CIOCPEventManager::AcquireSend()
{
	CLockGuard lockGuard(m_sendEventLock);
	if (m_sendEventPool.empty()) return nullptr;

	CSendEvent* event = m_sendEventPool.front();
	m_sendEventPool.pop_front();
	return event;
}

void CIOCPEventManager::ReleaseSend(CSendEvent* _event)
{
	if (_event == nullptr) return;
	_event->GetContext().overlapped = {};
	_event->Cleanup();

	CLockGuard lockGuard(m_sendEventLock);
	m_sendEventPool.push_back(_event);
}

CAcceptEvent* CIOCPEventManager::AcquireAccept()
{
	CLockGuard lockGuard(m_acceptEventLock);
	if (m_acceptEventPool.empty()) return nullptr;

	CAcceptEvent* event = m_acceptEventPool.front();
	m_acceptEventPool.pop_front();
	return event;
}

void CIOCPEventManager::ReleaseAccept(CAcceptEvent* _event)
{
	if (_event == nullptr) return;
	_event->GetContext().overlapped = {};
	_event->Cleanup();

	CLockGuard lockGuard(m_acceptEventLock);
	m_acceptEventPool.push_back(_event);
}
