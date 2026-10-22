#pragma once
#include "ChatManager.h"
#include "SessionController.h"
#include "PartyManager.h"
#include "../Utility_Core/JobPool.h"

class CPacketTask final : public CJob
{
private:
	std::vector<byte>	m_packet;

	CTCPConnection*		m_owner = nullptr;

	CJobPool*			m_jobPool;

	CSessionManager*	m_sessionManager;
	CChatManager*		m_chatManager;
	CSessionController*	m_sessionController;
	CPartyManager*		m_partyManager;

public:
	explicit	CPacketTask(CJobPool* _jobPool, CSessionManager* _sessionManager, CChatManager* _chatManager, CSessionController* _sessionController, CPartyManager* _partyManager = nullptr) noexcept;
	~CPacketTask() override = default;

	void		Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner) noexcept;
	void		Execute() override;

private:
	void		Cleanup();

	void		OnWorldChat();
	void		OnWhisper();
	void		OnPartyChat();

	void		OnParty();
	void		FlushPartyEvents();
};
