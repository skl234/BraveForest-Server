#pragma once
#include "Player.h"
#include "../Common/PACKET.h"

enum class eSessionState : uint8_t
{
	SelectCharacter = 0,
	PreparePending,
	ReservePending,
	LeavePending,
	Loading,
	EnterPending,
	StartPending,
	InZone,
	CancelPending,
	LogoutPending,
};

class CSession
{
private:
	uint64_t		m_sessionId;
	uint64_t		m_accountIndex = 0;

	bool			m_active = false;
	bool			m_closing = false;
	bool			m_inGame = false;

	CPlayer			m_player;

	uint64_t		m_zoneId = 0;
	uint64_t		m_channelId = 0;

	uint64_t		m_partyNum = 0;
	bool			m_partyMoving = false;

	uint64_t		m_targetZoneId = 0;
	uint64_t		m_targetChannelId = 0;

	eSessionState	m_state = eSessionState::SelectCharacter;
	ePacketType		m_requestType = ePacketType::C2S_SelectCharacter;
	eZoneResult		m_result = eZoneResult::Fail;

	PLAYER_DATA		m_transferData{};

public:
	explicit CSession(uint64_t _sessionId) noexcept;
	~CSession() = default;

	void				Initialize(uint64_t _accountIndex) noexcept;
	void				Cleanup() noexcept;

	uint64_t			GetSessionId() const noexcept;
	uint64_t			GetAccountIndex() const noexcept;
	PROXY_HEADER		GetHeader() const noexcept;
	CPlayer&			GetPlayer() noexcept;
	const CPlayer&		GetPlayer() const noexcept;

	bool				IsActive() const noexcept;
	bool				IsClosing() const noexcept;
	bool				IsInGame() const noexcept;
	void				StartClosing() noexcept;
	eSessionState		GetState() const noexcept;
	void				UpdateState(eSessionState _state) noexcept;

	uint64_t			GetZoneId() const noexcept;
	uint64_t			GetChannelId() const noexcept;
	void				UpdateZone(uint64_t _zoneId, uint64_t _channelId) noexcept;

	uint64_t			GetTargetZoneId() const noexcept;
	uint64_t			GetTargetChannelId() const noexcept;
	void				UpdateTarget(uint64_t _zoneId, uint64_t _channelId) noexcept;

	ePacketType			GetRequestType() const noexcept;
	void				UpdateRequest(ePacketType _type) noexcept;
	eZoneResult			GetResult() const noexcept;
	void				UpdateResult(eZoneResult _result) noexcept;
	PLAYER_DATA&		GetTransferData() noexcept;
	const PLAYER_DATA&	GetTransferData() const noexcept;

	uint64_t			GetPartyNum() const noexcept;
	void				UpdateParty(uint64_t _partyNum) noexcept;
	bool				IsPartyMoving() const noexcept;
	void				UpdatePartyMoving(bool _moving) noexcept;
};
