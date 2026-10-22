#pragma once
#include "Party.h"
#include "SessionManager.h"
#include <unordered_map>

struct PARTY_INVITE
{
	uint64_t		inviteNum;
	uint64_t		partyNum;
	uint64_t		createNum;
	PROXY_HEADER	inviter;
	PROXY_HEADER	target;
	uint64_t		characterIndex;
};

struct PARTY_APPLICATION
{
	uint64_t		requestNum;
	uint64_t		partyNum;
	uint64_t		createNum;
	PROXY_HEADER	applicant;
	uint64_t		characterIndex;
};

struct PARTY_LIST_ENTRY
{
	uint64_t						partyNum;
	uint64_t						createNum;
	uint64_t						leaderCharacterIndex;
	std::string						name;
	std::vector<PARTY_MEMBER_INFO>	memberList;
};

struct PARTY_EVENT
{
	ePacketType							type;
	std::vector<PROXY_HEADER>			clientList;
	uint64_t							partyNum = 0;
	uint64_t							createNum = 0;
	uint64_t							leaderCharacterIndex = 0;
	uint64_t							inviteNum = 0;
	uint64_t							requestNum = 0;
	uint64_t							page = 0;
	uint64_t							totalCount = 0;
	bool								active = false;
	ePartyResult						result = ePartyResult::Success;
	std::string							name;
	std::vector<PARTY_MEMBER_INFO>		memberList;
	PARTY_MEMBER_STATUS					status{};
	std::vector<PARTY_LIST_ENTRY>		partyList;
	std::vector<PARTY_APPLICATION_INFO>	applicationList;
};

class CPartyManager
{
private:
	CSessionManager*						m_sessionManager = nullptr;

	std::vector<CParty>						m_partyList;
	std::unordered_map<uint64_t, uint64_t>	m_characterPartyList;
	std::vector<uint64_t>					m_freeNumList;

	std::vector<PARTY_INVITE>				m_inviteList;
	std::vector<PARTY_APPLICATION>			m_applicationList;

	std::vector<PARTY_EVENT>				m_eventList;

	uint64_t								m_nextCreateNum = 1;
	uint64_t								m_nextInviteNum = 1;
	uint64_t								m_nextRequestNum = 1;

public:
	CPartyManager() = default;
	~CPartyManager() = default;

	void						Initialize(CSessionManager* _sessionManager);
	void						Cleanup() noexcept;

	ePartyResult				Create(CSession& _session, const std::string& _name);
	ePartyResult				GetPartyList(const CSession& _session, uint64_t _page);

	ePartyResult				Invite(CSession& _session, uint64_t _partyNum, const std::string& _name);
	ePartyResult				AnswerInvite(CSession& _session, uint64_t _inviteNum, bool _accept);

	ePartyResult				Apply(CSession& _session, uint64_t _partyNum, uint64_t _createNum);
	ePartyResult				CancelApplication(CSession& _session, uint64_t _requestNum);
	ePartyResult				AnswerApplication(CSession& _session, uint64_t _requestNum, bool _accept);

	ePartyResult				Leave(CSession& _session, uint64_t _partyNum);
	ePartyResult				Kick(CSession& _session, uint64_t _partyNum, uint64_t _targetId, uint64_t _characterIndex);
	ePartyResult				ChangeLeader(CSession& _session, uint64_t _partyNum, uint64_t _targetId, uint64_t _characterIndex);
	ePartyResult				Disband(CSession& _session, uint64_t _partyNum);

	void						RemoveSession(CSession& _session);
	void						DisconnectSession(CSession& _session);
	void						RefreshSession(CSession& _session, bool _zoneChanged = true);
	void						UpdateStatus(CSession& _session, const PACKET_S2S_PARTY_MEMBER_STATUS& _packet);

	CParty*						Find(uint64_t _partyNum) noexcept;
	const CParty*				Find(uint64_t _partyNum) const noexcept;
	void						GetClientList(uint64_t _partyNum, std::vector<PROXY_HEADER>& _list) const;
	static bool					CanRequest(const CSession& _session) noexcept;

	std::vector<PARTY_EVENT>	TakeEvents() noexcept;

private:
	ePartyResult				CheckLeader(const CSession& _session, uint64_t _partyNum) const noexcept;
	static bool					IsValidName(const std::string& _name) noexcept;

	CSession*					FindSession(const PARTY_MEMBER& _member) const noexcept;
	const PARTY_APPLICATION*	FindApplication(uint64_t _requestNum) const noexcept;
	const PARTY_APPLICATION*	FindSessionApplication(uint64_t _sessionId) const noexcept;

	PARTY_MEMBER				MakeMember(const CSession& _session) const noexcept;
	PARTY_MEMBER_STATUS			GetStatus(const CSession& _session) const noexcept;
	void						GetMemberInfo(const CParty& _party, std::vector<PARTY_MEMBER_INFO>& _list) const;
	void						RemoveMember(CParty& _party, uint64_t _characterIndex);

	void						CancelInvite(uint64_t _inviteNum, ePartyResult _reason);
	void						CancelPartyInvites(uint64_t _partyNum);
	void						RemoveApplication(uint64_t _requestNum, ePartyResult _reason);
	void						RemoveSessionRequests(const CSession& _session);
	void						RemovePartyRequests(uint64_t _partyNum);

	void						QueueInfo(uint64_t _partyNum);
	void						QueueSubscription(CSession& _session);
	void						QueueApplications(uint64_t _partyNum);
	void						QueueApplicationState(const PARTY_APPLICATION& _application, bool _active, ePartyResult _result);
};
