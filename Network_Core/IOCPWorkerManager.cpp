#include "IOCPWorkerManager.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <system_error>
#include "IOCPEvent.h"
#include "IOCPContext.h"

bool CIOCPWorkerManager::Initialize(const uint32_t _threadSize)
{
	assert(!m_isWork);
	if (m_isWork)
	{
		SetLastError(ERROR_BUSY);
		return false;
	}
	// kck 중복 호출 같은 디버그에서 충분히 체크 가능한 실수? 사항은 assert를 사용하는 방향으로
	// 이 구간 예외처리는 크게 걸고, 화면에 기록
	m_workerList.resize(_threadSize);
	return true;
}

bool CIOCPWorkerManager::StartWorker(const CIOCP& _iocp)
{
	assert(!m_isWork);
	if (m_isWork)
	{
		SetLastError(ERROR_BUSY);
		return false;
	}
	if (m_workerList.empty())
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	if (!_iocp.IsInit())
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}
	m_isWork = true;

	uint32_t size = static_cast<uint32_t>(m_workerList.size());
	for (uint32_t i = 0; i < size; ++i)
	{
		m_workerList[i] = std::thread([this, &_iocp]() { WorkerThreadProc(_iocp); });
	}

	return true;
}

bool CIOCPWorkerManager::AllJoin(const CIOCP& _iocp)
{
	const uint32_t size = static_cast<uint32_t>(m_workerList.size());
	for (uint32_t i = 0; i < size; ++i)
	{
		if (!m_workerList[i].joinable()) continue;
		if (!_iocp.Post(0, 0, nullptr)) return false;
	}
	try
	{
		for (uint32_t i = 0; i < size; ++i)
		{
			std::thread& thread = m_workerList[i];
			if (thread.joinable()) thread.join();
		}
	}
	catch (const std::system_error&)
	{
		return false;
	}

	m_isWork = false;
	return true;
}

bool CIOCPWorkerManager::IsRunning() const noexcept
{
	return m_isWork;
}

void CIOCPWorkerManager::WorkerThreadProc(const CIOCP& _iocp)
{
	int errorCode = ERROR_SUCCESS;
	try
	{
		while (true)
		{
			DWORD		byteTrans = 0;
			ULONG_PTR	key = 0;
			OVERLAPPED* overlapped = nullptr;

			const bool result = _iocp.GQCS(&byteTrans, &key, &overlapped);
			DWORD error = ERROR_SUCCESS;
			if (!result) error = GetLastError();
			if (overlapped == nullptr)
			{
				if (!result || key == 0) break;
				continue;
			}

			IOCP_CONTEXT* context = reinterpret_cast<IOCP_CONTEXT*>(overlapped);
			CIOCPEvent* event = context->event;

			assert(event != nullptr);
			if (result) event->Handle(*context, byteTrans);
			else event->HandleError(*context, error);
		}
		return;
	}
	catch (const std::bad_alloc&)
	{
		errorCode = ERROR_NOT_ENOUGH_MEMORY;
	}
	catch (const std::system_error& _error)
	{
		errorCode = _error.code().value();
	}
	catch (...)
	{
		errorCode = ERROR_UNHANDLED_EXCEPTION;
	}
	std::cout << "CIOCPWorkerManager::Worker error_code=" << errorCode << std::endl;
	std::exit(EXIT_FAILURE);
}
