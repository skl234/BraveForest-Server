#include "Session.h"

CSession::CSession(uint64_t _sessionId) noexcept :
	m_sessionId(_sessionId)
{
}

void CSession::Initialize(uint64_t _accountIndex) noexcept
{
	Cleanup();
	m_accountIndex = _accountIndex;
	m_active = true;
}

void CSession::Cleanup() noexcept
{
	m_active = false;
	m_closing = false;
	m_inGame = false;
	m_accountIndex = 0;
	m_player.Cleanup();
	m_zoneId = 0;
	m_channelId = 0;
	m_partyNum = 0;
	m_partyMoving = false;
	m_targetZoneId = 0;
	m_targetChannelId = 0;
	m_state = eSessionState::SelectCharacter;
	m_requestType = ePacketType::C2S_SelectCharacter;
	m_result = eZoneResult::Fail;
	m_transferData = {};
}


uint64_t CSession::GetSessionId() const noexcept
{
	return m_sessionId;
}

uint64_t CSession::GetAccountIndex() const noexcept
{
	return m_accountIndex;
}

PROXY_HEADER CSession::GetHeader() const noexcept
{
	return {m_sessionId, m_accountIndex};
}

CPlayer& CSession::GetPlayer() noexcept
{
	return m_player;
}

const CPlayer& CSession::GetPlayer() const noexcept
{
	return m_player;
}


bool CSession::IsActive() const noexcept
{
	return m_active;
}

bool CSession::IsClosing() const noexcept
{
	return m_closing;
}

bool CSession::IsInGame() const noexcept
{
	return m_inGame;
}

void CSession::StartClosing() noexcept
{
	m_closing = true;
}

eSessionState CSession::GetState() const noexcept
{
	return m_state;
}

void CSession::UpdateState(eSessionState _state) noexcept
{
	m_state = _state;
	// 존 이동 중에는 게임 입장 여부를 그대로 유지
	if (_state == eSessionState::InZone) m_inGame = true;
	if (_state == eSessionState::SelectCharacter) m_inGame = false;
}


uint64_t CSession::GetZoneId() const noexcept
{
	return m_zoneId;
}

uint64_t CSession::GetChannelId() const noexcept
{
	return m_channelId;
}

void CSession::UpdateZone(uint64_t _zoneId, uint64_t _channelId) noexcept
{
	m_zoneId = _zoneId;
	m_channelId = _channelId;
}


uint64_t CSession::GetTargetZoneId() const noexcept
{
	return m_targetZoneId;
}

uint64_t CSession::GetTargetChannelId() const noexcept
{
	return m_targetChannelId;
}

void CSession::UpdateTarget(uint64_t _zoneId, uint64_t _channelId) noexcept
{
	m_targetZoneId = _zoneId;
	m_targetChannelId = _channelId;
}


ePacketType CSession::GetRequestType() const noexcept
{
	return m_requestType;
}

void CSession::UpdateRequest(ePacketType _type) noexcept
{
	m_requestType = _type;
}

eZoneResult CSession::GetResult() const noexcept
{
	return m_result;
}

void CSession::UpdateResult(eZoneResult _result) noexcept
{
	m_result = _result;
}

PLAYER_DATA& CSession::GetTransferData() noexcept
{
	return m_transferData;
}

const PLAYER_DATA& CSession::GetTransferData() const noexcept
{
	return m_transferData;
}


uint64_t CSession::GetPartyNum() const noexcept
{
	return m_partyNum;
}

void CSession::UpdateParty(uint64_t _partyNum) noexcept
{
	m_partyNum = _partyNum;
}

bool CSession::IsPartyMoving() const noexcept
{
	return m_partyMoving;
}

void CSession::UpdatePartyMoving(bool _moving) noexcept
{
	m_partyMoving = _moving;
}
