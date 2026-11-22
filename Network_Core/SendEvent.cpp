#include "SendEvent.h"
#include <cassert>
#include "IOCPContext.h"
#include "TCPConnection.h"
#include <cstdint>
#include <utility>

void CSendEvent::Cleanup() noexcept
{
	m_owner = nullptr;
	m_sendPacket.reset();
}

void CSendEvent::Handle(IOCP_CONTEXT& _context, DWORD _byteTrans)
{
	assert(m_owner != nullptr);
	if (_context.event != this || m_owner == nullptr)
		return;

	CTCPConnection* owner = m_owner;
	owner->HandleSend(*this);
	owner->OnCompleteIO();
}

void CSendEvent::HandleError(IOCP_CONTEXT& _context, DWORD _error)
{
	assert(m_owner != nullptr);
	if (_context.event != this || m_owner == nullptr)
		return;

	CTCPConnection* owner = m_owner;
	owner->HandleSendError(*this, _error);
	owner->OnCompleteIO();
}


bool CSendEvent::Initialize(CTCPConnection* _owner, std::shared_ptr<std::vector<byte>> _packet) noexcept
{
	if (_owner == nullptr || _packet == nullptr)
		return false;
	if (m_owner != nullptr || m_sendPacket != nullptr)
		return false;
	if (_packet->empty() || _packet->size() > UINT32_MAX)
		return false;

	m_owner = _owner;
	m_sendPacket = std::move(_packet);

	return true;
}

WSABUF CSendEvent::GetBuffer() const noexcept
{
	WSABUF buffer = {};
	if (m_sendPacket == nullptr) return buffer;

	buffer.buf = reinterpret_cast<char*>(m_sendPacket->data());
	buffer.len = static_cast<ULONG>(m_sendPacket->size());

	return buffer;
}
