#pragma once
#include "IOCPEvent.h"

class CTCPConnection;

class CRecvEvent : public CIOCPEvent
{
private:
	CTCPConnection* m_owner = nullptr;
	WSABUF			m_buffer = {};

public:
	CRecvEvent() noexcept = default;
	~CRecvEvent() noexcept override = default;

	void Cleanup() noexcept override;
	void Handle(IOCP_CONTEXT& _context, DWORD _byteTrans) override;
	void HandleError(IOCP_CONTEXT& _context, DWORD _error) override;

	bool	Initialize(CTCPConnection* _owner, byte* _buffer, const ULONG _size) noexcept;
	WSABUF	GetBuffer() const noexcept;
};
