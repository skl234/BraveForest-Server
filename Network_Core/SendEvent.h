#pragma once
#include "IOCPEvent.h"
#include <memory>
#include <vector>

class CTCPConnection;

class CSendEvent : public CIOCPEvent
{
private:
	CTCPConnection* m_owner = nullptr;
	std::shared_ptr<std::vector<byte>> m_sendPacket;

public:
	CSendEvent() noexcept = default;
	~CSendEvent() noexcept override = default;

	void Cleanup() noexcept override;
	void Handle(IOCP_CONTEXT& _context, DWORD _byteTrans) override;
	void HandleError(IOCP_CONTEXT& _context, DWORD _error) override;

	bool	Initialize(CTCPConnection* _owner, std::shared_ptr<std::vector<byte>> _packet) noexcept;
	WSABUF	GetBuffer() const noexcept;
};
