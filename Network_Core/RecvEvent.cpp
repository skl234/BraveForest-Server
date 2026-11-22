#include "RecvEvent.h"
#include <cassert>
#include "IOCPContext.h"
#include "TCPConnection.h"

void CRecvEvent::Cleanup() noexcept
{
	m_owner = nullptr;
	m_buffer = {};
}

void CRecvEvent::Handle(IOCP_CONTEXT& _context, DWORD _byteTrans)
{
	assert(m_owner != nullptr);
	if (_context.event != this || m_owner == nullptr)
		return;

	CTCPConnection* owner = m_owner;
	owner->HandleRecv(*this, _byteTrans);
	owner->OnCompleteIO();
}

void CRecvEvent::HandleError(IOCP_CONTEXT& _context, DWORD _error)
{
	assert(m_owner != nullptr);
	if (_context.event != this || m_owner == nullptr)
		return;

	CTCPConnection* owner = m_owner;
	owner->HandleRecvError(*this, _error);
	owner->OnCompleteIO();
}

bool CRecvEvent::Initialize(CTCPConnection* _owner, byte* _buffer, const ULONG _size) noexcept
{
	if (_owner == nullptr || _buffer == nullptr || _size == 0)
		return false;
	if (m_owner != nullptr)
		return false;

	m_owner = _owner;
	m_buffer.buf = reinterpret_cast<char*>(_buffer);
	m_buffer.len = _size;
	return true;
}

WSABUF CRecvEvent::GetBuffer() const noexcept
{
	return m_buffer;
}
