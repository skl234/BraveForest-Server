#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "../Common/PACKET.h"

struct PARTY_MEMBER
{
	uint64_t			accountIndex = 0;
	PARTY_MEMBER_INFO	info{};
};

class CParty
{
private:
	uint64_t					m_partyNum = 0;
	uint64_t					m_createNum = 0;
	std::string					m_name;

	uint64_t					m_leaderCharacterIndex = 0;
	std::vector<PARTY_MEMBER>	m_memberList;

public:
	static constexpr uint64_t MAX_MEMBER_COUNT = 4;

	CParty() = default;
	~CParty() = default;

	void								Initialize(uint64_t _partyNum, uint64_t _createNum, const PARTY_MEMBER& _member, const std::string& _name);
	void								Cleanup() noexcept;

	bool								AddMember(const PARTY_MEMBER& _member);
	bool								RemoveMember(uint64_t _characterIndex) noexcept;
	bool								ChangeLeader(uint64_t _characterIndex) noexcept;

	PARTY_MEMBER*						FindMember(uint64_t _characterIndex) noexcept;
	const PARTY_MEMBER*					FindMember(uint64_t _characterIndex) const noexcept;
	bool								IsMember(uint64_t _characterIndex) const noexcept;
	bool								IsActive() const noexcept;

	uint64_t							GetNum() const noexcept;
	uint64_t							GetCreateNum() const noexcept;
	const std::string&					GetName() const noexcept;
	uint64_t							GetLeader() const noexcept;
	const std::vector<PARTY_MEMBER>&	GetMemberList() const noexcept;
};
