#include "PacketTask.h"
#include "ChannelManager.h"
#include "SessionManager.h"
#include "CharacterDB.h"
#include "../Common/PACKET.h"
#include "../Common/ZoneData.h"
#include "../Network_Core/TCPConnection.h"
#include <cstddef>

bool CPacketTask::OnWorldPacket()
{
	PROXY_PACKET_HEADER* header = reinterpret_cast<PROXY_PACKET_HEADER*>(m_packet.data());
	ePacketType type = header->packetHeader.type;
	CSession* session = m_sessionManager->Find(header->proxyHeader.sessionId);
	if (session != nullptr && session->GetAccountIndex() != header->proxyHeader.accountIndex) return false;
	if (type == ePacketType::S2S_SetParty)
	{
		if (m_packet.size() != sizeof(PACKET_S2S_SET_PARTY)) return false;
		const PACKET_S2S_SET_PARTY& recv = *reinterpret_cast<const PACKET_S2S_SET_PARTY*>(m_packet.data());
		CPlayer* player = nullptr;
		if (session != nullptr) player = session->GetPlayer();
		if (player != nullptr && player->GetField() != nullptr && !session->IsTransferPending() &&
			recv.zoneId == m_zoneId && recv.channelId == session->GetChannelId() && recv.characterIndex == player->GetCharacterIndex())
		{
			player->UpdateParty(recv.partyNum, m_zoneId);
			NotifyPartyState(session, m_owner);
		}
		else
		{
			PACKET_S2S_PARTY_MEMBER_STATUS send{};
			send.proxyHeader = recv.proxyHeader;
			send.packetHeader = { sizeof(send) - sizeof(PROXY_HEADER), ePacketType::S2S_PartyMemberStatus };
			send.partyNum = recv.partyNum;
			send.result = ePartyResult::NotFound;
			send.status = { recv.proxyHeader.sessionId, recv.characterIndex, 0, m_zoneId, recv.channelId, ePartyMemberState::Moving };
			m_owner->PostSend(MakeShared(send));
		}
		return false;
	}

	if (type == ePacketType::S2S_EnterZone)
	{
		uint64_t prefix = offsetof(PACKET_S2S_CHARACTER, name);
		if (m_packet.size() < prefix) return false;
		PACKET_S2S_CHARACTER* recv = reinterpret_cast<PACKET_S2S_CHARACTER*>(m_packet.data());
		if (recv->nameSize == 0 || recv->nameSize > 30 || m_packet.size() != prefix + recv->nameSize) return false;
		PACKET_S2S_ZONE send(ePacketType::S2S_EnterZoneResult);
		send.proxyHeader = header->proxyHeader;
		send.zoneId = m_zoneId;
		if (session != nullptr && session->IsReserved() && session->GetPlayer() == nullptr &&
			recv->playerData.accountIndex == session->GetAccountIndex() && recv->playerData.zoneId == m_zoneId &&
			recv->playerData.characterIndex != 0 && recv->playerData.level >= 1 && recv->playerData.level <= 20 &&
			static_cast<uint64_t>(recv->playerData.jobClass) <= static_cast<uint64_t>(eJobClass::Archer))
		{
			VECTOR3 position{recv->playerData.position.x, 0.0f, recv->playerData.position.z};
			if (!m_channel->GetField()->IsWalkable(position))
			{
				ZONE_INFO zone{};
				if (m_zoneData->FindZone(m_zoneId, zone)) position = {zone.enterPosition.x, 0.0f, zone.enterPosition.z};
			}
			CPlayer* player = m_channel->StagePlayer(recv->playerData.characterIndex, std::string(recv->name, recv->nameSize), static_cast<eClassState>(recv->playerData.jobClass), position, recv->playerData.level, recv->playerData.experience);
			if (player != nullptr)
			{
				session->SetReservation(m_channel->GetChannelId(), false);
				session->SetPlayer(recv->playerData.characterIndex, m_channel->GetChannelId(), player);
				if (recv->playerData.maxHP != 0 && recv->playerData.hp < player->GetMaxHP())
					player->TakeDamage(player->GetMaxHP() - recv->playerData.hp);
				player->UpdateRotationY(recv->playerData.rotationY);
				send.channelId = m_channel->GetChannelId();
				send.result = eZoneResult::Success;
				FillPlayerData(session, send.playerData);
			}
			else send.result = eZoneResult::InvalidPosition;
		}
		m_owner->PostSend(MakeShared(send));
		return false;
	}
	if (m_packet.size() != sizeof(PACKET_S2S_ZONE)) return false;
	PACKET_S2S_ZONE* recv = reinterpret_cast<PACKET_S2S_ZONE*>(m_packet.data());
	PACKET_S2S_ZONE send(type);
	send.proxyHeader = header->proxyHeader;
	send.zoneId = m_zoneId;

	if (type == ePacketType::S2S_ReserveZone)
	{
		send.packetHeader.type = ePacketType::S2S_ReserveZoneResult;
		send.result = eZoneResult::Full;
		bool replacement = false;
		if (session != nullptr && session->GetPlayer() != nullptr)
			replacement = recv->respawn && session->GetPlayer()->GetState() == ePlayerState::Die;
		if (session != nullptr && session->IsReserved()) send.result = eZoneResult::Fail;
		else if (session == nullptr || replacement)
		{
			bool reserved = false;
			if (replacement) reserved = m_channel->ReserveReplacement();
			else reserved = m_channel->ReservePlayer();
			if (!reserved)
			{
				CChannel* next = m_channelManager->SelectEnterChannel();
				if (next != nullptr && next != m_channel)
				{
					m_channel = next;
					next->PushJob(this);
					return true;
				}
			}
			if (reserved)
			{
				if (session == nullptr) session = m_sessionManager->Activate(header->proxyHeader.sessionId, header->proxyHeader.accountIndex);
				if (session == nullptr) m_channel->CancelPlayerReservation();
				else
				{
					session->SetReservation(m_channel->GetChannelId(), true);
					send.result = eZoneResult::Success;
					send.channelId = m_channel->GetChannelId();
				}
			}
		}
	}
	else if (type == ePacketType::S2S_PrepareZoneMove)
	{
		send.packetHeader.type = ePacketType::S2S_PrepareZoneMoveResult;
		if (session != nullptr && session->GetPlayer() != nullptr)
		{
			CPlayer* player = session->GetPlayer();
			FillPlayerData(session, send.playerData);
			if (recv->respawn && player->GetState() == ePlayerState::Die)
			{
				ZONE_INFO town{};
				if (m_zoneData->FindZone(TOWN_ZONE_ID, town) && town.isActive)
				{
					send.playerData.zoneId = town.zoneId;
					send.playerData.position = town.enterPosition;
					send.result = eZoneResult::Success;
				}
			}
			if (!recv->respawn && player->GetState() != ePlayerState::Die)
			{
				ZONE_PORTAL_INFO portal{};
				ZONE_INFO target{};
				if (m_zoneData->FindPortal(m_zoneId, recv->portalId, portal) && portal.isActive &&
					m_zoneData->FindZone(portal.targetZoneId, target) && target.isActive && portal.targetZoneId != m_zoneId)
				{
					send.playerData.zoneId = portal.targetZoneId;
					send.playerData.position = portal.enterPosition;
					send.result = eZoneResult::Success;
				}
			}
		}
	}
	else if (type == ePacketType::S2S_CancelEnterZone)
	{
		send.packetHeader.type = ePacketType::S2S_CancelEnterZoneResult;
		if (session != nullptr)
		{
			if (session->IsReserved())
			{
				m_channel->CancelPlayerReservation();
				session->SetReservation(m_channel->GetChannelId(), false);
			}
			CPlayer* player = session->GetPlayer();
			if (player != nullptr && player->GetField() == nullptr)
			{
				m_channel->RemovePlayer(player);
				session->ClearPlayer();
			}
			if (session->GetPlayer() == nullptr) m_sessionManager->Deactivate(header->proxyHeader.sessionId);
		}
		send.result = eZoneResult::Success;
	}
	else if (type == ePacketType::S2S_LeaveZone)
	{
		send.packetHeader.type = ePacketType::S2S_LeaveZoneResult;
		if (session == nullptr) send.result = eZoneResult::Success;
		else
		{
			CPlayer* player = session->GetPlayer();
			if (player == nullptr) send.result = eZoneResult::Success;
			else
			{
				FillPlayerData(session, send.playerData);
				PLAYER_DATA save = send.playerData;
				if (player->GetState() == ePlayerState::Die)
				{
					ZONE_INFO town{};
					if (m_zoneData->FindZone(TOWN_ZONE_ID, town))
					{
						save.zoneId = town.zoneId;
						save.position = town.enterPosition;
					}
				}
				if (m_characterDB->Save(save))
				{
					m_channel->RemovePlayer(player);
					session->ClearPlayer();
					FlushViewEventList(m_channel);
					send.result = eZoneResult::Success;
				}
			}
			if (send.result == eZoneResult::Success && !session->IsReserved()) m_sessionManager->Deactivate(header->proxyHeader.sessionId);
		}
	}
	else if (type == ePacketType::S2S_StartZone)
	{
		send.packetHeader.type = ePacketType::S2S_StartZoneResult;
		if (session != nullptr && session->GetPlayer() != nullptr && session->GetPlayer()->GetField() == nullptr)
		{
			PLAYER_DATA data{};
			FillPlayerData(session, data);
			if (m_characterDB->Save(data) && m_channel->PublishPlayer(session->GetPlayer()))
			{
				send.result = eZoneResult::Success;
				send.channelId = session->GetChannelId();
				send.playerData = data;
				m_owner->PostSend(MakeShared(send));
				SendEnter(session);
				return false;
			}
			m_channel->RemovePlayer(session->GetPlayer());
			session->ClearPlayer();
			m_sessionManager->Deactivate(header->proxyHeader.sessionId);
		}
	}
	else return false;
	m_owner->PostSend(MakeShared(send));
	return false;
}


void CPacketTask::FillPlayerData(CSession* _session, PLAYER_DATA& _data)
{
	_data = {};
	CPlayer* player = _session->GetPlayer();
	if (player == nullptr) return;
	_data.accountIndex = _session->GetAccountIndex();
	_data.characterIndex = player->GetCharacterIndex();
	_data.jobClass = static_cast<eJobClass>(player->GetClass());
	_data.level = player->GetLevel();
	_data.experience = player->GetExperience();
	_data.maxHP = player->GetMaxHP();
	_data.hp = player->GetHP();
	_data.zoneId = m_zoneId;
	_data.position = {player->GetPosition().x, 0.0f, player->GetPosition().z};
	_data.rotationY = player->GetRotationY();
}

void CPacketTask::NotifyWorld(CSession* _session, ePacketType _type)
{
	CPlayer* player = _session->GetPlayer();
	if (player == nullptr) return;
	PACKET_S2S_ZONE send(_type);
	send.proxyHeader = {_session->GetSessionId(), _session->GetAccountIndex()};
	send.zoneId = m_zoneId;
	send.channelId = _session->GetChannelId();
	send.result = eZoneResult::Success;
	send.playerData.accountIndex = _session->GetAccountIndex();
	send.playerData.characterIndex = player->GetCharacterIndex();
	send.playerData.level = player->GetLevel();
	m_owner->PostSend(MakeShared(send));
}

void CPacketTask::NotifyPartyState(CSession* _session, CTCPConnection* _owner, bool _moving)
{
	if (_session == nullptr || _owner == nullptr) return;
	CPlayer* player = _session->GetPlayer();
	if (player == nullptr || player->GetPartyNum() == 0) return;
	PACKET_S2S_PARTY_MEMBER_STATUS send{};
	send.proxyHeader = { _session->GetSessionId(), _session->GetAccountIndex() };
	send.packetHeader = { sizeof(send) - sizeof(PROXY_HEADER), ePacketType::S2S_PartyMemberStatus };
	send.partyNum = player->GetPartyNum();
	send.result = ePartyResult::Success;
	send.status = { _session->GetSessionId(), player->GetCharacterIndex(), player->GetLevel(),
		player->GetPartyZoneId(), _session->GetChannelId(), ePartyMemberState::Ready };
	if (_moving) send.status.state = ePartyMemberState::Moving;
	else if (player->GetHP() == 0) send.status.state = ePartyMemberState::Dead;
	send.status.hp = player->GetHP();
	send.status.maxHP = player->GetMaxHP();
	_owner->PostSend(MakeShared(send));
}
