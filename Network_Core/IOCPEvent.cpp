#include "IOCPEvent.h"

CIOCPEvent::CIOCPEvent() noexcept
{
	m_context.event = this;
}

IOCP_CONTEXT& CIOCPEvent::GetContext() noexcept
{
	return m_context;
}
