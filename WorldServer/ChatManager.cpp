#include "ChatManager.h"

void CChatManager::Initialize(const CSessionManager* _sessionManager, const CPartyManager* _partyManager) noexcept
{
	m_sessionManager = _sessionManager;
	m_partyManager = _partyManager;
}


const CSession* CChatManager::PrepareWorldChat(uint64_t _sessionId, uint64_t _accountIndex, std::vector<PROXY_HEADER>& _clientList) const
{
	_clientList.clear();
	if (m_sessionManager == nullptr) return nullptr;

	const CSession* sender = m_sessionManager->Find(_sessionId);
	if (sender == nullptr || sender->GetAccountIndex() != _accountIndex || sender->IsClosing()) return nullptr;
	if (!sender->IsInGame() || sender->GetState() != eSessionState::InZone) return nullptr;

	const std::string& name = sender->GetPlayer().GetName();
	if (sender->GetPlayer().GetCharacterIndex() == 0 || name.empty() || name.size() > UINT8_MAX) return nullptr;

	const std::vector<CSession>& sessionList = m_sessionManager->GetList();
	_clientList.reserve(sessionList.size());

	for (uint64_t i = 0; i < sessionList.size(); ++i)
	{
		const CSession& session = sessionList[i];
		if (!session.IsActive() || session.IsClosing() || !session.IsInGame()) continue;

		_clientList.push_back(session.GetHeader());
	}

	return sender;
}

const CSession* CChatManager::PreparePartyChat(uint64_t _sessionId, uint64_t _accountIndex, uint64_t _partyNum, std::vector<PROXY_HEADER>& _clientList) const
{
	_clientList.clear();
	if (m_sessionManager == nullptr || m_partyManager == nullptr || _partyNum == 0) return nullptr;
	const CSession* sender = m_sessionManager->Find(_sessionId);
	if (sender == nullptr || sender->GetAccountIndex() != _accountIndex || !CPartyManager::CanRequest(*sender) || sender->GetPartyNum() != _partyNum) return nullptr;
	m_partyManager->GetClientList(_partyNum, _clientList);
	return sender;
}

const bool CChatManager::PrepareWhisper(uint64_t _sessionId, uint64_t _accountIndex, const std::string& _name, const CSession*& _sender, const CSession*& _target, eWhisperResult& _result) const
{
	_sender = nullptr;
	_target = nullptr;
	_result = eWhisperResult::Fail;
	if (m_sessionManager == nullptr || _name.empty() || _name.size() > 30 || _name.find('\0') != std::string::npos) return false;

	const CSession* sender = m_sessionManager->Find(_sessionId);
	if (sender == nullptr || sender->GetAccountIndex() != _accountIndex || sender->IsClosing()) return false;

	_sender = sender;
	if (!sender->IsInGame() || sender->GetState() != eSessionState::InZone) return true;

	const std::string& name = sender->GetPlayer().GetName();
	if (sender->GetPlayer().GetCharacterIndex() == 0 || name.empty() || name.size() > 30) return false;

	const CSession* target = m_sessionManager->FindByName(_name);
	if (target == nullptr || target->IsClosing() || !target->IsInGame())
	{
		_result = eWhisperResult::NotFound;
		return true;
	}

	_target = target;
	_result = eWhisperResult::Success;

	return true;
}
