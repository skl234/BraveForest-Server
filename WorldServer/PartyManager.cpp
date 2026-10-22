#include "PartyManager.h"

void CPartyManager::Initialize(CSessionManager* _sessionManager)
{
	Cleanup();
	m_sessionManager = _sessionManager;
	m_partyList.resize(1);
}

void CPartyManager::Cleanup() noexcept
{
	if (m_sessionManager != nullptr)
	{
		const std::vector<CSession>& sessions = m_sessionManager->GetList();
		for (uint64_t i = 0; i < sessions.size(); ++i)
		{
			CSession* session = m_sessionManager->Find(i);
			if (session != nullptr) session->UpdateParty(0);
		}
	}
	m_partyList.clear();
	m_characterPartyList.clear();
	m_freeNumList.clear();
	m_inviteList.clear();
	m_applicationList.clear();
	m_eventList.clear();
}


ePartyResult CPartyManager::Create(CSession& _session, const std::string& _name)
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;
	if (_session.GetPartyNum() != 0) return ePartyResult::AlreadyInParty;
	if (!IsValidName(_name)) return ePartyResult::InvalidName;

	uint64_t number = m_partyList.size();
	if (!m_freeNumList.empty())
	{
		number = m_freeNumList.back();
		m_freeNumList.pop_back();
	}
	else
	{
		if (number > m_sessionManager->GetList().size()) return ePartyResult::Full;
		m_partyList.emplace_back();
	}

	m_partyList[number].Initialize(number, m_nextCreateNum++, MakeMember(_session), _name);
	m_characterPartyList[_session.GetPlayer().GetCharacterIndex()] = number;
	_session.UpdateParty(number);
	RemoveSessionRequests(_session);
	RefreshSession(_session);

	return ePartyResult::Success;
}

ePartyResult CPartyManager::GetPartyList(const CSession& _session, uint64_t _page)
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;

	PARTY_EVENT event{};
	event.type = ePacketType::S2C_PartyList;
	event.clientList.push_back(_session.GetHeader());
	for (uint64_t i = 1; i < m_partyList.size(); ++i)
	{
		if (m_partyList[i].IsActive()) ++event.totalCount;
	}

	uint64_t lastPage = 0;
	if (event.totalCount != 0) lastPage = (event.totalCount - 1) / 5;
	event.page = _page;
	if (event.page > lastPage) event.page = lastPage;

	uint64_t index = 0;
	for (uint64_t i = 1; i < m_partyList.size(); ++i)
	{
		const CParty& party = m_partyList[i];
		if (!party.IsActive()) continue;
		if (index++ < event.page * 5) continue;

		PARTY_LIST_ENTRY entry{};
		entry.partyNum = party.GetNum();
		entry.createNum = party.GetCreateNum();
		entry.leaderCharacterIndex = party.GetLeader();
		entry.name = party.GetName();
		GetMemberInfo(party, entry.memberList);
		event.partyList.push_back(std::move(entry));
		if (event.partyList.size() == 5) break;
	}

	m_eventList.push_back(std::move(event));
	return ePartyResult::Success;
}


ePartyResult CPartyManager::Invite(CSession& _session, uint64_t _partyNum, const std::string& _name)
{
	ePartyResult result = CheckLeader(_session, _partyNum);
	if (result != ePartyResult::Success) return result;
	if (Find(_partyNum)->GetMemberList().size() >= CParty::MAX_MEMBER_COUNT) return ePartyResult::Full;

	const CSession* target = m_sessionManager->FindByName(_name);
	if (target == nullptr || !target->IsInGame() || target->IsClosing()) return ePartyResult::NotFound;
	if (target == &_session) return ePartyResult::InvalidTarget;
	if (!CanRequest(*target)) return ePartyResult::InvalidState;
	if (target->GetPartyNum() != 0) return ePartyResult::AlreadyInParty;

	for (uint64_t i = 0; i < m_inviteList.size(); ++i)
	{
		if (m_inviteList[i].target.sessionId == target->GetSessionId()) return ePartyResult::InvitePending;
	}

	PARTY_INVITE invite{ m_nextInviteNum++, _partyNum, Find(_partyNum)->GetCreateNum(), _session.GetHeader(), target->GetHeader(), target->GetPlayer().GetCharacterIndex() };
	m_inviteList.push_back(invite);

	PARTY_EVENT event{};
	event.type = ePacketType::S2C_PartyInvite;
	event.clientList.push_back(target->GetHeader());
	event.partyNum = _partyNum;
	event.inviteNum = invite.inviteNum;
	event.name = _session.GetPlayer().GetName();
	m_eventList.push_back(std::move(event));
	return ePartyResult::Success;
}

ePartyResult CPartyManager::AnswerInvite(CSession& _session, uint64_t _inviteNum, bool _accept)
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;
	for (uint64_t i = 0; i < m_inviteList.size(); ++i)
	{
		const PARTY_INVITE invite = m_inviteList[i];
		if (invite.inviteNum != _inviteNum || invite.target.sessionId != _session.GetSessionId() || invite.target.accountIndex != _session.GetAccountIndex()) continue;
		if (invite.characterIndex != _session.GetPlayer().GetCharacterIndex()) return ePartyResult::InvalidTarget;

		if (!_accept)
		{
			CancelInvite(invite.inviteNum, ePartyResult::Declined);
			return ePartyResult::Success;
		}

		CSession* inviter = m_sessionManager->Find(invite.inviter.sessionId);
		CParty* party = Find(invite.partyNum);
		if (inviter == nullptr || inviter->GetAccountIndex() != invite.inviter.accountIndex || inviter->IsClosing() ||
			party == nullptr || party->GetCreateNum() != invite.createNum || party->GetLeader() != inviter->GetPlayer().GetCharacterIndex() || inviter->GetPartyNum() != invite.partyNum)
		{
			CancelInvite(invite.inviteNum, ePartyResult::Canceled);
			return ePartyResult::Canceled;
		}
		if (!CanRequest(*inviter)) return ePartyResult::InvalidState;
		if (_session.GetPartyNum() != 0)
		{
			CancelInvite(invite.inviteNum, ePartyResult::Canceled);
			return ePartyResult::AlreadyInParty;
		}
		if (!party->AddMember(MakeMember(_session)))
		{
			CancelInvite(invite.inviteNum, ePartyResult::Full);
			return ePartyResult::Full;
		}

		m_characterPartyList[_session.GetPlayer().GetCharacterIndex()] = invite.partyNum;
		_session.UpdateParty(invite.partyNum);
		CancelInvite(invite.inviteNum, ePartyResult::Success);
		RemoveSessionRequests(_session);
		RefreshSession(_session);
		return ePartyResult::Success;
	}
	return ePartyResult::NotFound;
}


ePartyResult CPartyManager::Apply(CSession& _session, uint64_t _partyNum, uint64_t _createNum)
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;
	if (_session.GetPartyNum() != 0) return ePartyResult::AlreadyInParty;
	const CParty* party = Find(_partyNum);
	if (party == nullptr || party->GetCreateNum() != _createNum) return ePartyResult::NotFound;
	if (party->GetMemberList().size() >= CParty::MAX_MEMBER_COUNT) return ePartyResult::Full;
	if (FindSessionApplication(_session.GetSessionId()) != nullptr) return ePartyResult::ApplicationPending;

	PARTY_APPLICATION application{ m_nextRequestNum++, _partyNum, _createNum, _session.GetHeader(), _session.GetPlayer().GetCharacterIndex() };
	m_applicationList.push_back(application);
	QueueApplicationState(application, true, ePartyResult::Success);
	QueueApplications(_partyNum);
	return ePartyResult::Success;
}

ePartyResult CPartyManager::CancelApplication(CSession& _session, uint64_t _requestNum)
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;
	const PARTY_APPLICATION* application = FindApplication(_requestNum);
	if (application == nullptr) return ePartyResult::ApplicationNotFound;
	if (application->applicant.sessionId != _session.GetSessionId() || application->applicant.accountIndex != _session.GetAccountIndex() || application->characterIndex != _session.GetPlayer().GetCharacterIndex()) return ePartyResult::InvalidTarget;
	RemoveApplication(_requestNum, ePartyResult::Canceled);
	return ePartyResult::Success;
}

ePartyResult CPartyManager::AnswerApplication(CSession& _session, uint64_t _requestNum, bool _accept)
{
	const PARTY_APPLICATION* found = FindApplication(_requestNum);
	if (found == nullptr) return ePartyResult::ApplicationNotFound;
	const PARTY_APPLICATION application = *found;
	ePartyResult result = CheckLeader(_session, application.partyNum);
	if (result != ePartyResult::Success) return result;
	CParty* party = Find(application.partyNum);
	if (party->GetCreateNum() != application.createNum) return ePartyResult::NotFound;
	if (!_accept)
	{
		RemoveApplication(_requestNum, ePartyResult::Declined);
		return ePartyResult::Success;
	}
	CSession* applicant = m_sessionManager->Find(application.applicant.sessionId);
	if (applicant == nullptr || applicant->GetAccountIndex() != application.applicant.accountIndex || applicant->GetPlayer().GetCharacterIndex() != application.characterIndex || applicant->IsClosing())
	{
		RemoveApplication(_requestNum, ePartyResult::NotFound);
		return ePartyResult::NotFound;
	}
	if (!CanRequest(*applicant)) return ePartyResult::InvalidState;
	if (applicant->GetPartyNum() != 0)
	{
		RemoveApplication(_requestNum, ePartyResult::AlreadyInParty);
		return ePartyResult::AlreadyInParty;
	}
	if (!party->AddMember(MakeMember(*applicant))) return ePartyResult::Full;
	m_characterPartyList[applicant->GetPlayer().GetCharacterIndex()] = party->GetNum();
	applicant->UpdateParty(party->GetNum());
	RemoveApplication(_requestNum, ePartyResult::Success);
	RemoveSessionRequests(*applicant);
	RefreshSession(*applicant);
	return ePartyResult::Success;
}


ePartyResult CPartyManager::Leave(CSession& _session, uint64_t _partyNum)
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;
	if (_partyNum == 0 || _partyNum != _session.GetPartyNum()) return ePartyResult::NotFound;
	RemoveSession(_session);
	return ePartyResult::Success;
}

ePartyResult CPartyManager::Kick(CSession& _session, uint64_t _partyNum, uint64_t _targetId, uint64_t _characterIndex)
{
	ePartyResult result = CheckLeader(_session, _partyNum);
	if (result != ePartyResult::Success) return result;
	CParty* party = Find(_partyNum);
	const PARTY_MEMBER* member = party->FindMember(_characterIndex);
	if (member == nullptr || _characterIndex == _session.GetPlayer().GetCharacterIndex() || member->info.status.sessionId != _targetId) return ePartyResult::InvalidTarget;
	RemoveMember(*party, _characterIndex);
	return ePartyResult::Success;
}

ePartyResult CPartyManager::ChangeLeader(CSession& _session, uint64_t _partyNum, uint64_t _targetId, uint64_t _characterIndex)
{
	ePartyResult result = CheckLeader(_session, _partyNum);
	if (result != ePartyResult::Success) return result;
	const CSession* target = m_sessionManager->Find(_targetId);
	if (target == nullptr || target == &_session || target->IsClosing() || target->GetPartyNum() != _partyNum ||
		target->GetPlayer().GetCharacterIndex() != _characterIndex) return ePartyResult::InvalidTarget;
	if (!Find(_partyNum)->ChangeLeader(_characterIndex)) return ePartyResult::InvalidTarget;
	CancelPartyInvites(_partyNum);
	PARTY_EVENT cleared{};
	cleared.type = ePacketType::S2C_PartyApplicationList;
	cleared.partyNum = _partyNum;
	cleared.createNum = Find(_partyNum)->GetCreateNum();
	cleared.clientList.push_back(_session.GetHeader());
	m_eventList.push_back(std::move(cleared));
	QueueInfo(_partyNum);
	QueueApplications(_partyNum);
	return ePartyResult::Success;
}

ePartyResult CPartyManager::Disband(CSession& _session, uint64_t _partyNum)
{
	ePartyResult result = CheckLeader(_session, _partyNum);
	if (result != ePartyResult::Success) return result;
	const std::vector<PARTY_MEMBER>& members = Find(_partyNum)->GetMemberList();
	RemovePartyRequests(_partyNum);
	PARTY_EVENT cleared{};
	cleared.type = ePacketType::S2C_PartyInfo;
	GetClientList(_partyNum, cleared.clientList);
	m_eventList.push_back(std::move(cleared));
	for (uint64_t i = 0; i < members.size(); ++i)
	{
		m_characterPartyList.erase(members[i].info.status.characterIndex);
		CSession* member = FindSession(members[i]);
		if (member == nullptr) continue;
		member->UpdateParty(0);
		QueueSubscription(*member);
	}
	Find(_partyNum)->Cleanup();
	m_freeNumList.push_back(_partyNum);
	return ePartyResult::Success;
}


void CPartyManager::RemoveSession(CSession& _session)
{
	RemoveSessionRequests(_session);
	CParty* party = Find(_session.GetPartyNum());
	if (party == nullptr) return;
	RemoveMember(*party, _session.GetPlayer().GetCharacterIndex());
}

void CPartyManager::DisconnectSession(CSession& _session)
{
	RemoveSessionRequests(_session);
	CParty* party = Find(_session.GetPartyNum());
	if (party == nullptr) return;
	PARTY_MEMBER* member = party->FindMember(_session.GetPlayer().GetCharacterIndex());
	if (member == nullptr || member->accountIndex != _session.GetAccountIndex() || member->info.status.sessionId != _session.GetSessionId()) return;

	// 세션 번호는 재사용되므로 파티에는 캐릭터 정보만 남김
	member->info.status.sessionId = UINT64_MAX;
	member->info.status.state = ePartyMemberState::Offline;
	member->info.status.hp = 0;
	member->info.status.maxHP = 0;
	_session.UpdateParty(0);

	PARTY_EVENT cleared{};
	cleared.type = ePacketType::S2C_PartyInfo;
	cleared.clientList.push_back(_session.GetHeader());
	m_eventList.push_back(std::move(cleared));
	QueueSubscription(_session);
	QueueInfo(party->GetNum());
}

void CPartyManager::RefreshSession(CSession& _session, bool _zoneChanged)
{
	if (_session.IsClosing() || !_session.IsInGame())
	{
		DisconnectSession(_session);
		return;
	}
	if (_session.GetPartyNum() == 0 && _session.GetState() == eSessionState::InZone)
	{
		std::unordered_map<uint64_t, uint64_t>::iterator iter = m_characterPartyList.find(_session.GetPlayer().GetCharacterIndex());
		if (iter != m_characterPartyList.end())
		{
			CParty* party = Find(iter->second);
			if (party == nullptr) return;
			PARTY_MEMBER* member = party->FindMember(iter->first);
			if (member == nullptr || member->accountIndex != _session.GetAccountIndex()) return;
			if (member->info.status.state != ePartyMemberState::Offline && member->info.status.sessionId != _session.GetSessionId()) return;
			_session.UpdateParty(party->GetNum());
			*member = MakeMember(_session);
		}
	}
	CParty* party = Find(_session.GetPartyNum());
	if (party == nullptr) return;
	if (_zoneChanged && _session.GetState() == eSessionState::InZone)
	{
		_session.UpdatePartyMoving(false);
		QueueSubscription(_session);
	}
	PARTY_MEMBER* member = party->FindMember(_session.GetPlayer().GetCharacterIndex());
	if (member != nullptr) member->info.status = GetStatus(_session);
	QueueInfo(_session.GetPartyNum());
	QueueApplications(_session.GetPartyNum());
}

void CPartyManager::UpdateStatus(CSession& _session, const PACKET_S2S_PARTY_MEMBER_STATUS& _packet)
{
	const PARTY_MEMBER_STATUS& status = _packet.status;
	if (!CanRequest(_session) || _packet.result != ePartyResult::Success || _packet.partyNum == 0 || _packet.partyNum != _session.GetPartyNum()) return;
	if (_packet.proxyHeader.accountIndex != _session.GetAccountIndex() || status.sessionId != _session.GetSessionId() ||
		status.characterIndex != _session.GetPlayer().GetCharacterIndex()) return;
	if (status.zoneId != _session.GetZoneId() || status.channelId != _session.GetChannelId()) return;

	if (status.state == ePartyMemberState::Moving)
	{
		_session.UpdatePartyMoving(true);
		RefreshSession(_session, false);
		return;
	}
	if (status.level == 0 || status.level > 20 || status.maxHP == 0 || status.hp > status.maxHP) return;
	if (status.state != ePartyMemberState::Ready && status.state != ePartyMemberState::Dead) return;
	if ((status.state == ePartyMemberState::Dead) != (status.hp == 0)) return;

	CParty* party = Find(_session.GetPartyNum());
	if (party == nullptr) return;
	PARTY_MEMBER* member = party->FindMember(status.characterIndex);
	if (member == nullptr || FindSession(*member) != &_session) return;

	const PARTY_MEMBER_STATUS& old = member->info.status;
	if (old.level == status.level && old.hp == status.hp && old.maxHP == status.maxHP && old.state == status.state &&
		old.zoneId == status.zoneId && old.channelId == status.channelId) return;
	_session.GetPlayer().UpdateLevel(status.level);
	member->info.status = status;

	PARTY_EVENT event{};
	event.type = ePacketType::S2C_PartyMemberStatus;
	event.partyNum = _session.GetPartyNum();
	event.status = GetStatus(_session);
	GetClientList(event.partyNum, event.clientList);
	m_eventList.push_back(std::move(event));
}


CParty* CPartyManager::Find(uint64_t _partyNum) noexcept
{
	if (_partyNum == 0 || _partyNum >= m_partyList.size() || !m_partyList[_partyNum].IsActive()) return nullptr;
	return &m_partyList[_partyNum];
}

const CParty* CPartyManager::Find(uint64_t _partyNum) const noexcept
{
	if (_partyNum == 0 || _partyNum >= m_partyList.size() || !m_partyList[_partyNum].IsActive()) return nullptr;
	return &m_partyList[_partyNum];
}

void CPartyManager::GetClientList(uint64_t _partyNum, std::vector<PROXY_HEADER>& _list) const
{
	_list.clear();
	const CParty* party = Find(_partyNum);
	if (party == nullptr) return;
	const std::vector<PARTY_MEMBER>& members = party->GetMemberList();
	for (uint64_t i = 0; i < members.size(); ++i)
	{
		const CSession* session = FindSession(members[i]);
		if (session == nullptr || !session->IsInGame() || session->GetPartyNum() != _partyNum) continue;
		_list.push_back(session->GetHeader());
	}
}

bool CPartyManager::CanRequest(const CSession& _session) noexcept
{
	return _session.IsActive() && !_session.IsClosing() && _session.IsInGame() &&
		_session.GetState() == eSessionState::InZone && !_session.IsPartyMoving() && _session.GetPlayer().GetCharacterIndex() != 0;
}


std::vector<PARTY_EVENT> CPartyManager::TakeEvents() noexcept
{
	std::vector<PARTY_EVENT> events;
	events.swap(m_eventList);
	return events;
}


ePartyResult CPartyManager::CheckLeader(const CSession& _session, uint64_t _partyNum) const noexcept
{
	if (!CanRequest(_session)) return ePartyResult::InvalidState;
	const CParty* party = Find(_partyNum);
	if (party == nullptr || _session.GetPartyNum() != _partyNum || !party->IsMember(_session.GetPlayer().GetCharacterIndex())) return ePartyResult::NotFound;
	if (party->GetLeader() != _session.GetPlayer().GetCharacterIndex()) return ePartyResult::NotLeader;
	return ePartyResult::Success;
}

bool CPartyManager::IsValidName(const std::string& _name) noexcept
{
	if (_name.empty() || _name.size() > 60) return false;
	wchar_t text[60]{};
	const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, _name.data(), static_cast<int>(_name.size()), text, 60);
	if (size == 0) return false;
	uint64_t count = 0;
	bool visible = false;
	for (int i = 0; i < size; ++i)
	{
		if (text[i] < 32 || text[i] == 127 || text[i] == L'<' || text[i] == L'>') return false;
		WORD type = 0;
		if (!GetStringTypeW(CT_CTYPE1, &text[i], 1, &type)) return false;
		if ((type & C1_SPACE) == 0) visible = true;
		if (text[i] >= 0xDC00 && text[i] <= 0xDFFF) continue;
		++count;
	}
	return visible && count <= 15;
}


CSession* CPartyManager::FindSession(const PARTY_MEMBER& _member) const noexcept
{
	if (_member.info.status.state == ePartyMemberState::Offline) return nullptr;
	CSession* session = m_sessionManager->Find(_member.info.status.sessionId);
	if (session == nullptr || session->IsClosing() || session->GetAccountIndex() != _member.accountIndex ||
		session->GetPlayer().GetCharacterIndex() != _member.info.status.characterIndex) return nullptr;
	return session;
}

const PARTY_APPLICATION* CPartyManager::FindApplication(uint64_t _requestNum) const noexcept
{
	for (uint64_t i = 0; i < m_applicationList.size(); ++i)
	{
		if (m_applicationList[i].requestNum == _requestNum) return &m_applicationList[i];
	}
	return nullptr;
}

const PARTY_APPLICATION* CPartyManager::FindSessionApplication(uint64_t _sessionId) const noexcept
{
	for (uint64_t i = 0; i < m_applicationList.size(); ++i)
	{
		if (m_applicationList[i].applicant.sessionId == _sessionId) return &m_applicationList[i];
	}
	return nullptr;
}


PARTY_MEMBER CPartyManager::MakeMember(const CSession& _session) const noexcept
{
	PARTY_MEMBER member{};
	member.accountIndex = _session.GetAccountIndex();
	member.info.status = GetStatus(_session);
	member.info.jobClass = _session.GetPlayer().GetClass();
	const std::string& name = _session.GetPlayer().GetName();
	member.info.nameSize = static_cast<uint8_t>(name.size());
	if (name.size() <= sizeof(member.info.name)) memcpy(member.info.name, name.data(), name.size());
	return member;
}

PARTY_MEMBER_STATUS CPartyManager::GetStatus(const CSession& _session) const noexcept
{
	const CPlayer& player = _session.GetPlayer();
	PARTY_MEMBER_STATUS status{};
	status.sessionId = _session.GetSessionId();
	status.characterIndex = player.GetCharacterIndex();
	status.level = player.GetLevel();
	status.zoneId = _session.GetZoneId();
	status.channelId = _session.GetChannelId();
	const CParty* party = Find(_session.GetPartyNum());
	if (party != nullptr)
	{
		const PARTY_MEMBER* member = party->FindMember(player.GetCharacterIndex());
		if (member != nullptr && member->info.status.state != ePartyMemberState::Offline && member->info.status.sessionId == status.sessionId)
		{
			status.hp = member->info.status.hp;
			status.maxHP = member->info.status.maxHP;
			if (status.maxHP != 0 && status.hp == 0) status.state = ePartyMemberState::Dead;
		}
	}
	if (_session.GetState() != eSessionState::InZone || _session.IsPartyMoving()) status.state = ePartyMemberState::Moving;
	return status;
}

void CPartyManager::GetMemberInfo(const CParty& _party, std::vector<PARTY_MEMBER_INFO>& _list) const
{
	_list.clear();
	const std::vector<PARTY_MEMBER>& members = _party.GetMemberList();
	for (uint64_t i = 0; i < members.size(); ++i)
	{
		_list.push_back(members[i].info);
	}
}

void CPartyManager::RemoveMember(CParty& _party, uint64_t _characterIndex)
{
	const PARTY_MEMBER* member = _party.FindMember(_characterIndex);
	if (member == nullptr) return;
	CSession* session = FindSession(*member);
	const uint64_t number = _party.GetNum();
	const bool leader = _party.GetLeader() == _characterIndex;
	if (session != nullptr)
	{
		RemoveSessionRequests(*session);
		session->UpdateParty(0);
		PARTY_EVENT cleared{};
		cleared.type = ePacketType::S2C_PartyInfo;
		cleared.clientList.push_back(session->GetHeader());
		m_eventList.push_back(std::move(cleared));
		QueueSubscription(*session);
	}
	_party.RemoveMember(_characterIndex);
	m_characterPartyList.erase(_characterIndex);
	if (leader) CancelPartyInvites(number);
	if (_party.GetMemberList().empty())
	{
		RemovePartyRequests(number);
		_party.Cleanup();
		m_freeNumList.push_back(number);
	}
	else
	{
		QueueInfo(number);
		QueueApplications(number);
	}
}


void CPartyManager::CancelInvite(uint64_t _inviteNum, ePartyResult _reason)
{
	for (uint64_t i = 0; i < m_inviteList.size(); ++i)
	{
		if (m_inviteList[i].inviteNum != _inviteNum) continue;
		PARTY_EVENT event{};
		event.type = ePacketType::S2C_CancelPartyInvite;
		event.inviteNum = _inviteNum;
		event.result = _reason;
		event.clientList.push_back(m_inviteList[i].target);
		m_eventList.push_back(std::move(event));
		m_inviteList.erase(m_inviteList.begin() + i);
		return;
	}
}

void CPartyManager::CancelPartyInvites(uint64_t _partyNum)
{
	for (uint64_t i = m_inviteList.size(); i > 0; --i)
	{
		if (m_inviteList[i - 1].partyNum == _partyNum) CancelInvite(m_inviteList[i - 1].inviteNum, ePartyResult::Canceled);
	}
}

void CPartyManager::RemoveApplication(uint64_t _requestNum, ePartyResult _reason)
{
	for (uint64_t i = 0; i < m_applicationList.size(); ++i)
	{
		if (m_applicationList[i].requestNum != _requestNum) continue;
		const PARTY_APPLICATION application = m_applicationList[i];
		m_applicationList.erase(m_applicationList.begin() + i);
		QueueApplicationState(application, false, _reason);
		QueueApplications(application.partyNum);
		return;
	}
}

void CPartyManager::RemoveSessionRequests(const CSession& _session)
{
	for (uint64_t i = m_inviteList.size(); i > 0; --i)
	{
		const PARTY_INVITE& invite = m_inviteList[i - 1];
		if (invite.inviter.sessionId == _session.GetSessionId() || invite.target.sessionId == _session.GetSessionId()) CancelInvite(invite.inviteNum, ePartyResult::Canceled);
	}
	const PARTY_APPLICATION* application = FindSessionApplication(_session.GetSessionId());
	if (application != nullptr) RemoveApplication(application->requestNum, ePartyResult::Canceled);
}

void CPartyManager::RemovePartyRequests(uint64_t _partyNum)
{
	CancelPartyInvites(_partyNum);
	for (uint64_t i = m_applicationList.size(); i > 0; --i)
	{
		if (m_applicationList[i - 1].partyNum != _partyNum) continue;
		QueueApplicationState(m_applicationList[i - 1], false, ePartyResult::Canceled);
		m_applicationList.erase(m_applicationList.begin() + i - 1);
	}
}


void CPartyManager::QueueInfo(uint64_t _partyNum)
{
	const CParty* party = Find(_partyNum);
	if (party == nullptr) return;
	PARTY_EVENT event{};
	event.type = ePacketType::S2C_PartyInfo;
	event.partyNum = _partyNum;
	event.createNum = party->GetCreateNum();
	event.name = party->GetName();
	event.leaderCharacterIndex = party->GetLeader();
	GetClientList(_partyNum, event.clientList);
	GetMemberInfo(*party, event.memberList);
	m_eventList.push_back(std::move(event));
}

void CPartyManager::QueueSubscription(CSession& _session)
{
	if (_session.GetZoneId() == 0 || _session.GetPlayer().GetCharacterIndex() == 0) return;
	PARTY_EVENT event{};
	event.type = ePacketType::S2S_SetParty;
	event.clientList.push_back(_session.GetHeader());
	event.partyNum = _session.GetPartyNum();
	event.status = GetStatus(_session);
	m_eventList.push_back(std::move(event));
}

void CPartyManager::QueueApplications(uint64_t _partyNum)
{
	const CParty* party = Find(_partyNum);
	if (party == nullptr) return;
	const PARTY_MEMBER* member = party->FindMember(party->GetLeader());
	if (member == nullptr) return;
	const CSession* leader = FindSession(*member);
	if (leader == nullptr || !leader->IsInGame()) return;
	PARTY_EVENT event{};
	event.type = ePacketType::S2C_PartyApplicationList;
	event.partyNum = _partyNum;
	event.createNum = party->GetCreateNum();
	event.clientList.push_back(leader->GetHeader());
	for (uint64_t i = 0; i < m_applicationList.size(); ++i)
	{
		const PARTY_APPLICATION& application = m_applicationList[i];
		if (application.partyNum != _partyNum || application.createNum != party->GetCreateNum()) continue;
		const CSession* session = m_sessionManager->Find(application.applicant.sessionId);
		if (session == nullptr || session->IsClosing() || session->GetAccountIndex() != application.applicant.accountIndex || session->GetPlayer().GetCharacterIndex() != application.characterIndex) continue;
		const CPlayer& player = session->GetPlayer();
		if (player.GetName().size() > 30) continue;
		PARTY_APPLICATION_INFO info{};
		info.requestNum = application.requestNum;
		info.level = player.GetLevel();
		info.jobClass = player.GetClass();
		info.nameSize = static_cast<uint8_t>(player.GetName().size());
		memcpy(info.name, player.GetName().data(), info.nameSize);
		event.applicationList.push_back(info);
	}
	m_eventList.push_back(std::move(event));
}

void CPartyManager::QueueApplicationState(const PARTY_APPLICATION& _application, bool _active, ePartyResult _result)
{
	PARTY_EVENT event{};
	event.type = ePacketType::S2C_PartyApplicationState;
	event.clientList.push_back(_application.applicant);
	event.requestNum = _application.requestNum;
	event.partyNum = _application.partyNum;
	event.createNum = _application.createNum;
	event.active = _active;
	event.result = _result;
	m_eventList.push_back(std::move(event));
}
