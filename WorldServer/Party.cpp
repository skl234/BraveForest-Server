#include "Party.h"

void CParty::Initialize(uint64_t _partyNum, uint64_t _createNum, const PARTY_MEMBER& _member, const std::string& _name)
{
	Cleanup();
	m_memberList.push_back(_member);
	m_partyNum = _partyNum;
	m_createNum = _createNum;
	m_name = _name;
	m_leaderCharacterIndex = _member.info.status.characterIndex;
}

void CParty::Cleanup() noexcept
{
	m_partyNum = 0;
	m_createNum = 0;
	m_name.clear();
	m_leaderCharacterIndex = 0;
	m_memberList.clear();
}


bool CParty::AddMember(const PARTY_MEMBER& _member)
{
	if (!IsActive() || m_memberList.size() >= MAX_MEMBER_COUNT || IsMember(_member.info.status.characterIndex)) return false;
	m_memberList.push_back(_member);
	return true;
}

bool CParty::RemoveMember(uint64_t _characterIndex) noexcept
{
	for (uint64_t i = 0; i < m_memberList.size(); ++i)
	{
		if (m_memberList[i].info.status.characterIndex != _characterIndex) continue;
		m_memberList.erase(m_memberList.begin() + i);
		if (!m_memberList.empty() && m_leaderCharacterIndex == _characterIndex) m_leaderCharacterIndex = m_memberList[0].info.status.characterIndex;
		return true;
	}
	return false;
}

bool CParty::ChangeLeader(uint64_t _characterIndex) noexcept
{
	if (!IsMember(_characterIndex)) return false;
	m_leaderCharacterIndex = _characterIndex;
	return true;
}


PARTY_MEMBER* CParty::FindMember(uint64_t _characterIndex) noexcept
{
	for (uint64_t i = 0; i < m_memberList.size(); ++i)
	{
		if (m_memberList[i].info.status.characterIndex == _characterIndex) return &m_memberList[i];
	}
	return nullptr;
}

const PARTY_MEMBER* CParty::FindMember(uint64_t _characterIndex) const noexcept
{
	for (uint64_t i = 0; i < m_memberList.size(); ++i)
	{
		if (m_memberList[i].info.status.characterIndex == _characterIndex) return &m_memberList[i];
	}
	return nullptr;
}

bool CParty::IsMember(uint64_t _characterIndex) const noexcept
{
	return FindMember(_characterIndex) != nullptr;
}

bool CParty::IsActive() const noexcept
{
	return m_partyNum != 0;
}


uint64_t CParty::GetNum() const noexcept
{
	return m_partyNum;
}

uint64_t CParty::GetCreateNum() const noexcept
{
	return m_createNum;
}

const std::string& CParty::GetName() const noexcept
{
	return m_name;
}

uint64_t CParty::GetLeader() const noexcept
{
	return m_leaderCharacterIndex;
}

const std::vector<PARTY_MEMBER>& CParty::GetMemberList() const noexcept
{
	return m_memberList;
}
