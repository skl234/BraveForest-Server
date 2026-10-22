#pragma once
#include "SessionManager.h"
#include "PartyManager.h"

class CChatManager
{
private:
	const CSessionManager*	m_sessionManager = nullptr;
	const CPartyManager*	m_partyManager = nullptr;

public:
	CChatManager() = default;
	~CChatManager() = default;

	void			Initialize(const CSessionManager* _sessionManager, const CPartyManager* _partyManager) noexcept;

	const CSession*	PrepareWorldChat(uint64_t _sessionId, uint64_t _accountIndex, std::vector<PROXY_HEADER>& _clientList) const;
	const CSession*	PreparePartyChat(uint64_t _sessionId, uint64_t _accountIndex, uint64_t _partyNum, std::vector<PROXY_HEADER>& _clientList) const;
	const bool		PrepareWhisper(uint64_t _sessionId, uint64_t _accountIndex, const std::string& _name, const CSession*& _sender, const CSession*& _target, eWhisperResult& _result) const;
};
