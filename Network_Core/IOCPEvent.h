#pragma once
#include <WinSock2.h>
#include "IOCPContext.h"
#include "../Utility_Core/UtilityMacros.h"

class CIOCPEvent
{
public:
	DELETE_COPY_MOVE(CIOCPEvent);

private:
	IOCP_CONTEXT m_context;

public:
	CIOCPEvent() noexcept;
	virtual ~CIOCPEvent() noexcept = default;

	IOCP_CONTEXT& GetContext() noexcept;

	virtual void Cleanup() noexcept = 0;
	virtual void Handle(IOCP_CONTEXT& _context, DWORD _byteTrans) = 0;
	virtual void HandleError(IOCP_CONTEXT& _context, DWORD _error) = 0;
};
