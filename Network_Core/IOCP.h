#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "../Utility_Core/UtilityMacros.h"

class CIOCP
{
public:
	DELETE_COPY_MOVE(CIOCP);

private:
	HANDLE m_completionPort = NULL;

public:
	CIOCP() noexcept = default;
	~CIOCP() noexcept;

	bool Initialize(const DWORD _concurrency) noexcept;
	bool IsInit() const noexcept;

	bool Regist(const HANDLE _handle, const ULONG_PTR _completionKey) const noexcept;
	bool GQCS(DWORD* const _byteTrans, ULONG_PTR* const _key, OVERLAPPED** const _overlapped, const DWORD _timeout = INFINITE) const noexcept;
	bool Post(const DWORD _byteTrans, const ULONG_PTR _key, OVERLAPPED* const _overlapped) const noexcept;
	void Close() noexcept;
};
