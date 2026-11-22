#include "IOCP.h"

CIOCP::~CIOCP() noexcept
{
	Close();
}

bool CIOCP::Initialize(const DWORD _concurrency) noexcept
{
	if (m_completionPort != NULL)
	{
		SetLastError(ERROR_ALREADY_INITIALIZED);
		return false;
	}
	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _concurrency);
	return m_completionPort != NULL;
}

bool CIOCP::IsInit() const noexcept
{
	return m_completionPort != NULL;
}

bool CIOCP::Regist(const HANDLE _handle, const ULONG_PTR _completionKey) const noexcept
{
	if (m_completionPort == NULL)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}
	return CreateIoCompletionPort(_handle, m_completionPort, _completionKey, 0) != NULL;
}

bool CIOCP::GQCS(DWORD* const _byteTrans, ULONG_PTR* const _key, OVERLAPPED** const _overlapped, const DWORD _timeout) const noexcept
{
	return GetQueuedCompletionStatus(m_completionPort, _byteTrans, _key, _overlapped, _timeout) != FALSE;
}

bool CIOCP::Post(const DWORD _byteTrans, const ULONG_PTR _key, OVERLAPPED* const _overlapped) const noexcept
{
	return PostQueuedCompletionStatus(m_completionPort, _byteTrans, _key, _overlapped) != FALSE;
}

void CIOCP::Close() noexcept
{
	if (m_completionPort != NULL) CloseHandle(m_completionPort);
	m_completionPort = NULL;
}
