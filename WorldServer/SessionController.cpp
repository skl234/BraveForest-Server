#include "SessionController.h"
#include <cstddef>
#include "PartyManager.h"
#include <iostream>

void CSessionController::Initialize(CSessionManager* _sessionManager, CTCPConnection* _proxyConnection, const CCharacterDB* _characterDB, const CZoneData* _zoneData) noexcept
{
	m_sessionManager = _sessionManager;
	m_proxyConnection = _proxyConnection;
	m_characterDB = _characterDB;
	m_zoneData = _zoneData;
}

void CSessionController::SetPartyManager(CPartyManager* _partyManager) noexcept
{
	m_partyManager = _partyManager;
}


void CSessionController::Process(const std::vector<byte>& _packet)
{
	if (_packet.size() < sizeof(PROXY_PACKET_HEADER)) return;
	const PROXY_PACKET_HEADER& header = *reinterpret_cast<const PROXY_PACKET_HEADER*>(_packet.data());
	if (header.packetHeader.size + sizeof(PROXY_HEADER) != _packet.size()) return;
	const ePacketType type = header.packetHeader.type;
	if (type == ePacketType::S2S_EnterWorld && _packet.size() == sizeof(PACKET_S2S_ENTER_WORLD))
	{
		PACKET_S2C_LOGIN send;
		send.proxyHeader = header.proxyHeader;
		send.result = m_sessionManager->Register(header.proxyHeader.sessionId, header.proxyHeader.accountIndex);
		m_proxyConnection->PostSend(MakeShared(send));
		return;
	}
	CSession* session = m_sessionManager->Find(header.proxyHeader.sessionId);
	if (session == nullptr || session->GetAccountIndex() != header.proxyHeader.accountIndex) return;
	if (type == ePacketType::C2S_Logout || type == ePacketType::S2S_LeaveWorld)
	{
		Close(*session);
		return;
	}
	if (type == ePacketType::S2C_Logout && _packet.size() == sizeof(PACKET_S2C_LOGOUT) && session->GetState() == eSessionState::LogoutPending)
	{
		m_proxyConnection->PostSend(std::make_shared<std::vector<byte>>(_packet));
		m_sessionManager->Unregister(header.proxyHeader.sessionId, header.proxyHeader.accountIndex);
		return;
	}
	if (type == ePacketType::S2S_SendServerFail && _packet.size() == sizeof(PACKET_S2S_ZONE))
	{
		const PACKET_S2S_ZONE& failed = *reinterpret_cast<const PACKET_S2S_ZONE*>(_packet.data());
		// 연결되지 않은 목적지 예약은 기존 존을 유지한 채 실패.
		if (session->GetState() == eSessionState::ReservePending)
		{
			PACKET_S2S_ZONE result = failed;
			result.packetHeader.type = ePacketType::S2S_ReserveZoneResult;
			OnZone(*session, result);
		}
		else
		{
			std::cerr << "World: backend unavailable, session " << session->GetSessionId() << "\n";
		}
		return;
	}
	if (_packet.size() == sizeof(PACKET_S2S_ZONE) && static_cast<uint16_t>(type) >= 3000)
	{
		OnZone(*session, *reinterpret_cast<const PACKET_S2S_ZONE*>(_packet.data()));
		return;
	}
	if (session->IsClosing()) return;
	if (type == ePacketType::C2S_CharacterList)
	{
		if (session->GetState() != eSessionState::SelectCharacter || _packet.size() != sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_CHARACTERLIST)) return;
		OnCharacterList(*session);
		return;
	}
	if (type == ePacketType::C2S_CreateCharacter)
	{
		if (session->GetState() != eSessionState::SelectCharacter) return;
		OnCreateCharacter(*session, _packet);
		return;
	}
	if (type == ePacketType::C2S_LeaveZone && _packet.size() == sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_LEAVE_ZONE))
	{
		if (session->GetState() != eSessionState::InZone) return;
		session->UpdateRequest(type);
		session->UpdateState(eSessionState::LeavePending);
		SetRoute(*session, 0);
		SendZone(*session, ePacketType::S2S_LeaveZone, session->GetZoneId());
		return;
	}
	if (type == ePacketType::C2S_SelectCharacter && _packet.size() == sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_SELECT_CHARACTER))
	{
		if (session->GetState() != eSessionState::SelectCharacter) return;
		const PACKET_C2S_SELECT_CHARACTER& request = *reinterpret_cast<const PACKET_C2S_SELECT_CHARACTER*>(_packet.data() + sizeof(PROXY_HEADER));
		OnSelectCharacter(*session, request.characterIndex);
		return;
	}
	if ((type == ePacketType::C2S_ChangeZone || type == ePacketType::C2S_Respawn) && session->GetState() == eSessionState::InZone)
	{
		uint64_t portalId = 0;
		if (type == ePacketType::C2S_ChangeZone)
		{
			if (_packet.size() != sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_CHANGE_ZONE)) return;
			portalId = reinterpret_cast<const PACKET_C2S_CHANGE_ZONE*>(_packet.data() + sizeof(PROXY_HEADER))->portalId;
		}
		else if (_packet.size() != sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_RESPAWN)) return;
		session->UpdateRequest(type);
		session->UpdateState(eSessionState::PreparePending);
		PACKET_S2S_ZONE send(ePacketType::S2S_PrepareZoneMove);
		send.proxyHeader = session->GetHeader();
		send.zoneId = session->GetZoneId();
		send.portalId = portalId;
		send.respawn = type == ePacketType::C2S_Respawn;
		SendServer(eBackServerType::Zone, send.zoneId, MakeShared(send));
		return;
	}
	if (type == ePacketType::C2S_EnterZone && session->GetState() == eSessionState::Loading)
	{
		if (_packet.size() != sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_ENTER_ZONE)) return;
		const PACKET_C2S_ENTER_ZONE* request = reinterpret_cast<const PACKET_C2S_ENTER_ZONE*>(_packet.data() + sizeof(PROXY_HEADER));
		if (request->characterIndex != session->GetPlayer().GetCharacterIndex()) return;
		session->UpdateRequest(type);
		session->UpdateState(eSessionState::EnterPending);
		const std::string& name = session->GetPlayer().GetName();
		const uint64_t size = offsetof(PACKET_S2S_CHARACTER, name) + name.size();
		std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
		PACKET_S2S_CHARACTER* send = reinterpret_cast<PACKET_S2S_CHARACTER*>(buffer->data());
		send->proxyHeader = session->GetHeader();
		send->packetHeader = {static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2S_EnterZone};
		send->result = eZoneResult::Success;
		send->playerData = session->GetTransferData();
		send->nameSize = static_cast<uint8_t>(name.size());
		memcpy(send->name, name.data(), name.size());
		SendServer(eBackServerType::Zone, session->GetTargetZoneId(), buffer);
	}
}


void CSessionController::OnCharacterList(const CSession& _session) const
{
	std::vector<CHARACTER> characterList;
	if (m_characterDB == nullptr || !m_characterDB->LoadList(_session.GetAccountIndex(), characterList)) return;

	const uint64_t size = sizeof(PACKET_S2C_CHARACTERLIST) + sizeof(CHARACTER) * characterList.size();
	std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
	PACKET_S2C_CHARACTERLIST* send = reinterpret_cast<PACKET_S2C_CHARACTERLIST*>(buffer->data());
	send->proxyHeader = _session.GetHeader();
	send->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2C_CharacterList };
	send->characterCount = static_cast<byte>(characterList.size());
	if (!characterList.empty()) memcpy(buffer->data() + sizeof(PACKET_S2C_CHARACTERLIST), characterList.data(), sizeof(CHARACTER) * characterList.size());
	m_proxyConnection->PostSend(buffer);
}

void CSessionController::OnCreateCharacter(const CSession& _session, const std::vector<byte>& _packet) const
{
	PACKET_S2C_CREATE_CHARACTER fail;
	fail.proxyHeader = _session.GetHeader();
	const uint64_t prefix = sizeof(PROXY_HEADER) + sizeof(PACKET_C2S_CREATE_CHARACTER);
	if (_packet.size() < prefix)
	{
		m_proxyConnection->PostSend(MakeShared(fail));
		return;
	}

	const PACKET_C2S_CREATE_CHARACTER& request = *reinterpret_cast<const PACKET_C2S_CREATE_CHARACTER*>(_packet.data() + sizeof(PROXY_HEADER));
	if (request.nameLen == 0 || request.nameLen > 30 || _packet.size() != prefix + request.nameLen ||
		(request.jobClass != eJobClass::Warrior && request.jobClass != eJobClass::Archer))
	{
		m_proxyConnection->PostSend(MakeShared(fail));
		return;
	}

	ZONE_INFO startZone{};
	CHARACTER character{};
	bool result = m_characterDB != nullptr && m_zoneData != nullptr && m_zoneData->FindStartZone(startZone);
	if (result)
	{
		const std::string name(reinterpret_cast<const char*>(_packet.data() + prefix), request.nameLen);
		result = m_characterDB->Create(_session.GetAccountIndex(), request.jobClass, name, startZone, character);
	}
	if (!result)
	{
		m_proxyConnection->PostSend(MakeShared(fail));
		return;
	}

	const uint64_t size = sizeof(PACKET_S2C_CREATE_CHARACTER) + sizeof(CHARACTER);
	std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
	PACKET_S2C_CREATE_CHARACTER* send = reinterpret_cast<PACKET_S2C_CREATE_CHARACTER*>(buffer->data());
	send->proxyHeader = _session.GetHeader();
	send->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2C_CreateCharacter };
	send->result = true;
	memcpy(buffer->data() + sizeof(PACKET_S2C_CREATE_CHARACTER), &character, sizeof(character));
	m_proxyConnection->PostSend(buffer);
}

void CSessionController::OnSelectCharacter(CSession& _session, uint64_t _characterIndex)
{
	_session.UpdateRequest(ePacketType::C2S_SelectCharacter);
	PLAYER_DATA data{};
	std::string name;
	ZONE_INFO zone{};
	bool result = m_characterDB != nullptr && m_zoneData != nullptr;
	if (result) result = m_characterDB->Load(_session.GetAccountIndex(), _characterIndex, data, name);
	if (result) result = m_zoneData->FindZone(data.zoneId, zone) && zone.isActive;
	if (!result || data.accountIndex != _session.GetAccountIndex() || data.characterIndex != _characterIndex || name.empty())
	{
		SelectCharacter(_session);
		SendResult(_session, eZoneResult::InvalidCharacter);
		return;
	}

	if (!m_sessionManager->RegisterPlayer(_session.GetSessionId(), data, name))
	{
		SelectCharacter(_session);
		SendResult(_session, eZoneResult::InvalidCharacter);
		return;
	}
	_session.GetTransferData() = data;
	_session.UpdateTarget(data.zoneId, 0);
	Reserve(_session);
}

void CSessionController::OnZone(CSession& _session, const PACKET_S2S_ZONE& _packet)
{
	const ePacketType type = _packet.packetHeader.type;
	const eSessionState state = _session.GetState();
	if (type == ePacketType::S2S_PlayerLevel || type == ePacketType::S2S_ChangeChannel)
	{
		if (_packet.zoneId != _session.GetZoneId() || _packet.playerData.characterIndex != _session.GetPlayer().GetCharacterIndex()) return;
		if (type == ePacketType::S2S_PlayerLevel) _session.GetPlayer().UpdateLevel(_packet.playerData.level);
		if (type == ePacketType::S2S_ChangeChannel) _session.UpdateZone(_packet.zoneId, _packet.channelId);
		return;
	}

	if (type == ePacketType::S2S_PrepareZoneMoveResult && state == eSessionState::PreparePending)
	{
		if (_session.IsClosing())
		{
			ContinueClose(_session);
			return;
		}
		if (_packet.result != eZoneResult::Success)
		{
			_session.UpdateState(eSessionState::InZone);
			SendResult(_session, _packet.result);
			return;
		}
		_session.GetTransferData() = _packet.playerData;
		_session.UpdateTarget(_packet.playerData.zoneId, 0);
		Reserve(_session);
		return;
	}

	if (type == ePacketType::S2S_ReserveZoneResult && state == eSessionState::ReservePending)
	{
		if (_packet.result != eZoneResult::Success)
		{
			_session.UpdateTarget(0, 0);
			if (_session.IsClosing())
			{
				ContinueClose(_session);
				return;
			}
			if (_session.GetZoneId() == 0) SelectCharacter(_session);
			else _session.UpdateState(eSessionState::InZone);
			SendResult(_session, _packet.result);
			return;
		}
		_session.UpdateTarget(_packet.zoneId, _packet.channelId);
		if (_session.IsClosing())
		{
			ContinueClose(_session);
			return;
		}
		if (_session.GetZoneId() != 0)
		{
			SetRoute(_session, 0);
			_session.UpdateState(eSessionState::LeavePending);
			SendZone(_session, ePacketType::S2S_LeaveZone, _session.GetZoneId());
		}
		else
		{
			_session.UpdateState(eSessionState::Loading);
			SendResult(_session, eZoneResult::Success);
		}
		return;
	}

	if (type == ePacketType::S2S_LeaveZoneResult && state == eSessionState::LeavePending)
	{
		if (_packet.result != eZoneResult::Success)
		{
			_session.UpdateResult(_packet.result);
			if (_session.GetTargetChannelId() != 0)
			{
				_session.UpdateState(eSessionState::CancelPending);
				SendZone(_session, ePacketType::S2S_CancelEnterZone, _session.GetTargetZoneId());
			}
			else
			{
				// 저장 실패 시 Player를 버리지 않음. 같은 종료 요청으로 재시도 가능.
				_session.UpdateState(eSessionState::InZone);
				std::cerr << "World: Zone leave/save failed, session " << _session.GetSessionId() << "\n";
				if (!_session.IsClosing())
				{
					SetRoute(_session, _session.GetZoneId());
					SendResult(_session, _packet.result);
				}
			}
			return;
		}
		_session.UpdateZone(0, 0);
		if (_packet.playerData.characterIndex != 0)
		{
			PLAYER_DATA data = _packet.playerData;
			data.zoneId = _session.GetTransferData().zoneId;
			data.position = _session.GetTransferData().position;
			if (_session.GetRequestType() == ePacketType::C2S_Respawn) data.hp = data.maxHP;
			_session.GetTransferData() = data;
			_session.GetPlayer().UpdateLevel(data.level);
		}
		if (_session.IsClosing())
		{
			ContinueClose(_session);
			return;
		}
		if (_session.GetRequestType() == ePacketType::C2S_LeaveZone) SelectCharacter(_session);
		else _session.UpdateState(eSessionState::Loading);
		SendResult(_session, eZoneResult::Success);
		return;
	}
	if (type == ePacketType::S2S_EnterZoneResult && state == eSessionState::EnterPending)
	{
		if (_packet.result != eZoneResult::Success)
		{
			_session.UpdateResult(_packet.result);
			_session.UpdateState(eSessionState::CancelPending);
			SendZone(_session, ePacketType::S2S_CancelEnterZone, _session.GetTargetZoneId());
			return;
		}
		if (_session.IsClosing())
		{
			ContinueClose(_session);
			return;
		}
		_session.UpdateZone(_packet.zoneId, _packet.channelId);
		_session.GetPlayer().UpdateLevel(_packet.playerData.level);
		SetRoute(_session, _packet.zoneId);
		_session.UpdateState(eSessionState::StartPending);
		SendZone(_session, ePacketType::S2S_StartZone, _packet.zoneId);
		return;
	}
	if (type == ePacketType::S2S_StartZoneResult && state == eSessionState::StartPending)
	{
		_session.UpdateTarget(0, 0);
		if (_packet.result != eZoneResult::Success)
		{
			_session.UpdateZone(0, 0);
			SetRoute(_session, 0);
			SelectCharacter(_session);
			SendResult(_session, _packet.result);
		}
		else
		{
			_session.UpdateState(eSessionState::InZone);
			_session.GetTransferData() = {};
		}
		if (_session.IsClosing()) ContinueClose(_session);
		return;
	}
	if (type == ePacketType::S2S_CancelEnterZoneResult && state == eSessionState::CancelPending)
	{
		if (_packet.result != eZoneResult::Success) return;
		_session.UpdateTarget(0, 0);
		if (_session.IsClosing())
		{
			ContinueClose(_session);
			return;
		}
		if (_session.GetZoneId() == 0) SelectCharacter(_session);
		else _session.UpdateState(eSessionState::InZone);
		SetRoute(_session, _session.GetZoneId());
		SendResult(_session, _session.GetResult());
	}
}


void CSessionController::SelectCharacter(CSession& _session) const
{
	if (m_partyManager != nullptr) m_partyManager->DisconnectSession(_session);
	m_sessionManager->UnregisterPlayer(_session.GetSessionId());
	_session.UpdateState(eSessionState::SelectCharacter);
}

void CSessionController::Reserve(CSession& _session)
{
	_session.UpdateState(eSessionState::ReservePending);
	SendZone(_session, ePacketType::S2S_ReserveZone, _session.GetTargetZoneId());
}

void CSessionController::Close(CSession& _session)
{
	_session.StartClosing();
	if (m_partyManager != nullptr) m_partyManager->DisconnectSession(_session);
	SetRoute(_session, 0);
	const eSessionState state = _session.GetState();
	if (state == eSessionState::SelectCharacter || state == eSessionState::InZone || state == eSessionState::Loading)
		ContinueClose(_session);
}

void CSessionController::ContinueClose(CSession& _session)
{
	if (_session.GetTargetChannelId() != 0)
	{
		_session.UpdateState(eSessionState::CancelPending);
		SendZone(_session, ePacketType::S2S_CancelEnterZone, _session.GetTargetZoneId());
		return;
	}

	if (_session.GetZoneId() != 0)
	{
		_session.UpdateState(eSessionState::LeavePending);
		SendZone(_session, ePacketType::S2S_LeaveZone, _session.GetZoneId());
		return;
	}

	_session.UpdateState(eSessionState::LogoutPending);
	PACKET_C2S_LOGOUT logout;
	std::shared_ptr<std::vector<byte>> packet = MakeShared(logout);
	SendServer(eBackServerType::Login, 0, std::make_shared<std::vector<byte>>(AttachProxyHeader(*packet, _session.GetHeader())));
}


void CSessionController::SetRoute(const CSession& _session, uint64_t _zoneId) const
{
	PACKET_S2S_SET_ZONE send{_session.GetHeader(), {sizeof(PACKET_S2S_SET_ZONE) - sizeof(PROXY_HEADER), ePacketType::S2S_SetZone}, _zoneId};
	m_proxyConnection->PostSend(MakeShared(send));
}

bool CSessionController::SendServer(eBackServerType _type, uint64_t _zoneId, const std::shared_ptr<std::vector<byte>>& _packet) const
{
	const uint64_t prefix = offsetof(PACKET_S2S_SEND_SERVER, packet);
	const uint64_t size = prefix + _packet->size();
	if (size - sizeof(PROXY_HEADER) > UINT16_MAX) return false;
	std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
	PACKET_S2S_SEND_SERVER* send = reinterpret_cast<PACKET_S2S_SEND_SERVER*>(buffer->data());
	send->proxyHeader = *reinterpret_cast<const PROXY_HEADER*>(_packet->data());
	send->packetHeader = {static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2S_SendServer};
	send->serverType = _type;
	send->zoneId = _zoneId;
	memcpy(send->packet, _packet->data(), _packet->size());
	return m_proxyConnection->PostSend(buffer);
}

void CSessionController::SendZone(CSession& _session, ePacketType _type, uint64_t _zoneId) const
{
	PACKET_S2S_ZONE send(_type);
	send.proxyHeader = _session.GetHeader();
	send.zoneId = _zoneId;
	send.channelId = _session.GetTargetChannelId();
	send.respawn = _session.GetRequestType() == ePacketType::C2S_Respawn;
	send.playerData = _session.GetTransferData();
	SendServer(eBackServerType::Zone, _zoneId, MakeShared(send));
}

void CSessionController::SendResult(CSession& _session, eZoneResult _result) const
{
	if (_session.IsClosing()) return;

	if (_session.GetRequestType() == ePacketType::C2S_SelectCharacter)
	{
		PACKET_S2C_SELECT_CHARACTER send;
		send.proxyHeader = _session.GetHeader();
		send.result = _result;
		send.playerData = _session.GetTransferData();
		m_proxyConnection->PostSend(MakeShared(send));
	}

	if (_session.GetRequestType() == ePacketType::C2S_ChangeZone)
	{
		PACKET_S2C_CHANGE_ZONE send;
		send.proxyHeader = _session.GetHeader();
		send.result = _result;
		send.zoneId = _session.GetTargetZoneId();
		send.position = _session.GetTransferData().position;
		m_proxyConnection->PostSend(MakeShared(send));
	}

	if (_session.GetRequestType() == ePacketType::C2S_Respawn)
	{
		PACKET_S2C_RESPAWN send;
		send.proxyHeader = _session.GetHeader();
		send.result = _result;
		send.zoneId = _session.GetTargetZoneId();
		send.position = _session.GetTransferData().position;
		m_proxyConnection->PostSend(MakeShared(send));
	}

	if (_session.GetRequestType() == ePacketType::C2S_LeaveZone)
	{
		PACKET_S2C_LEAVE_ZONE send;
		send.proxyHeader = _session.GetHeader();
		send.result = _result;
		m_proxyConnection->PostSend(MakeShared(send));
	}

	if (_session.GetRequestType() == ePacketType::C2S_EnterZone)
	{
		PACKET_S2C_ENTER_ZONE send{};
		send.proxyHeader = _session.GetHeader();
		send.packetHeader = {sizeof(send) - sizeof(PROXY_HEADER), ePacketType::S2C_EnterZone};
		send.result = _result;
		m_proxyConnection->PostSend(MakeShared(send));
	}
}
