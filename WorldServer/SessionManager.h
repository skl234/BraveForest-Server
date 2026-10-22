#pragma once
#include <vector>
#include <unordered_map>
#include "Session.h"

class CSessionManager
{
private:
	std::vector<CSession>						m_sessionList;
	std::unordered_map<std::string, uint64_t>	m_nameMap;

public:
	CSessionManager() = default;
	~CSessionManager() = default;

	void							Initialize(uint64_t _size);
	void							Cleanup() noexcept;

	bool							Register(uint64_t _sessionId, uint64_t _accountIndex) noexcept;
	bool							Unregister(uint64_t _sessionId, uint64_t _accountIndex) noexcept;

	bool							RegisterPlayer(uint64_t _sessionId, const PLAYER_DATA& _data, const std::string& _name);
	void							UnregisterPlayer(uint64_t _sessionId) noexcept;

	CSession*						Find(uint64_t _sessionId) noexcept;
	const CSession*					Find(uint64_t _sessionId) const noexcept;
	const CSession*					FindByName(const std::string& _name) const;
	const std::vector<CSession>&	GetList() const noexcept;
};
