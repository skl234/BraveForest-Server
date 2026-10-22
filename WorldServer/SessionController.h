#pragma once
#include "SessionManager.h"
#include "../Network_Core/TCPConnection.h"
#include "CharacterDB.h"
class CPartyManager;

class CSessionController
{
private:
	CSessionManager*	m_sessionManager = nullptr;
	CTCPConnection*		m_proxyConnection = nullptr;
	CPartyManager*		m_partyManager = nullptr;

	const CCharacterDB*	m_characterDB = nullptr;
	const CZoneData*		m_zoneData = nullptr;

public:
	CSessionController() = default;
	~CSessionController() = default;

	void	Initialize(CSessionManager* _sessionManager, CTCPConnection* _proxyConnection, const CCharacterDB* _characterDB, const CZoneData* _zoneData) noexcept;
	void	SetPartyManager(CPartyManager* _partyManager) noexcept;

	void	Process(const std::vector<byte>& _packet);

private:
	void	OnCharacterList(const CSession& _session) const;
	void	OnCreateCharacter(const CSession& _session, const std::vector<byte>& _packet) const;
	void	OnSelectCharacter(CSession& _session, uint64_t _characterIndex);
	void	OnZone(CSession& _session, const PACKET_S2S_ZONE& _packet);

	void	SelectCharacter(CSession& _session) const;
	void	Reserve(CSession& _session);
	void	Close(CSession& _session);
	void	ContinueClose(CSession& _session);

	void	SetRoute(const CSession& _session, uint64_t _zoneId) const;
	bool	SendServer(eBackServerType _type, uint64_t _zoneId, const std::shared_ptr<std::vector<byte>>& _packet) const;
	void	SendZone(CSession& _session, ePacketType _type, uint64_t _zoneId) const;
	void	SendResult(CSession& _session, eZoneResult _result) const;
};
