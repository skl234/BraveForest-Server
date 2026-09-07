#include "PacketTask.h"
#include "ChannelManager.h"
#include "SessionManager.h"
#include "../Common/ZoneData.h"
#include "CharacterDB.h"
#include "CombatDamageCalculator.h"
#include "../Common/PACKET.h"
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/JobPool.h"

CPacketTask::CPacketTask(CJobPool* _jobPool, CSessionManager* _sessionManager, CChannelManager* _channelManager, CZoneData* _zoneData, CCharacterDB* _characterDB, uint64_t _zoneId) :
	m_owner(nullptr),
	m_generation(0),
	m_jobPool(_jobPool),
	m_sessionManager(_sessionManager),
	m_channelManager(_channelManager),
	m_zoneData(_zoneData),
	m_characterDB(_characterDB),
	m_zoneId(_zoneId),
	m_channel(nullptr),
	m_channelTransferEnter(false),
	m_transferData{}
{
}

void CPacketTask::Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation, CChannel* _channel)
{
	m_packet = std::move(_packet);
	m_owner = _owner;
	m_generation = _generation;
	m_channel = _channel;
	m_channelTransferEnter = false;
	m_transferData = {};
}

void CPacketTask::Execute()
{
	if (m_owner == nullptr || m_owner->GetGeneration() != m_generation)
	{
		Cleanup();
		return;
	}

	// 프록시 연결은 유지하고 잘못된 패킷만 무시
	if (m_packet.size() < sizeof(PROXY_HEADER) + sizeof(PACKET_HEADER))
	{
		Cleanup();
		return;
	}
	PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(m_packet.data() + sizeof(PROXY_HEADER));
	if (!IsValidPacket(header))
	{
		Cleanup();
		return;
	}
	if (header->type == ePacketType::C2S_EnterZone) OnEnterZone();
	if (header->type == ePacketType::C2S_LeaveZone) OnLeaveZone();
	if (header->type == ePacketType::C2S_Move) OnMove();
	if (header->type == ePacketType::C2S_AttackStart) OnAttackStart();
	if (header->type == ePacketType::C2S_AttackExecute) OnAttackExecute();
	if (header->type == ePacketType::C2S_AttackCancel) OnAttackCancel();
	if (header->type == ePacketType::C2S_UsePotion) OnUsePotion();
	if (header->type == ePacketType::C2S_ChannelList) OnChannelList();
	if (header->type == ePacketType::C2S_Chat) OnChat();
	if (header->type == ePacketType::C2S_ChangeZone) OnChangeZone();
	if (header->type == ePacketType::C2S_Respawn) OnRespawn();
	if (header->type == ePacketType::C2S_ChangeChannel)
	{
		if (OnChangeChannel()) return;
	}
	Cleanup();
}

void CPacketTask::OnEnterZone()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_C2S_ENTER_ZONE* recv = reinterpret_cast<PACKET_C2S_ENTER_ZONE*>(m_packet.data() + sizeof(PROXY_HEADER));
	CChannel* channel = m_channel;
	if (channel == nullptr)
	{
		SendEnterFail(proxy, eZoneResult::Full);
		return;
	}

	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) session = m_sessionManager->Activate(proxy->sessionId, proxy->accountIndex);
	if (session == nullptr || session->GetPlayer() != nullptr)
	{
		SendEnterFail(proxy, eZoneResult::Fail);
		return;
	}
	PLAYER_DATA data{};
	std::string name;
	if (!m_characterDB->Load(proxy->accountIndex, recv->characterIndex, data, name))
	{
		SendEnterFail(proxy, eZoneResult::InvalidCharacter);
		return;
	}
	if (data.zoneId != m_zoneId)
	{
		SendEnterFail(proxy, eZoneResult::Fail);
		return;
	}
	VECTOR3 position{data.position.x, 0.0f, data.position.z};
	CPlayer* player = channel->CreatePlayer(data.characterIndex, name, static_cast<eClassState>(data.jobClass), position, data.level, data.experience);
	if (player == nullptr)
	{
		// 맵 변경으로 예전 저장 위치를 사용할 수 없으면 지정 입장 위치로 복구
		ZONE_INFO zoneInfo{};
		if (m_zoneData->FindZone(m_zoneId, zoneInfo) && zoneInfo.isActive)
		{
			position = {zoneInfo.enterPosition.x, 0.0f, zoneInfo.enterPosition.z};
			player = channel->CreatePlayer(data.characterIndex, name, static_cast<eClassState>(data.jobClass), position, data.level, data.experience);
		}
	}
	if (player == nullptr)
	{
		SendEnterFail(proxy, eZoneResult::InvalidPosition);
		return;
	}
	session->SetPlayer(data.characterIndex, channel->GetChannelId(), player);
	uint64_t objectCount = static_cast<uint64_t>(player->GetVisibleObjectList().size()) + 1;
	uint64_t packetSize = sizeof(PACKET_S2C_ENTER_ZONE) + sizeof(FIELD_OBJECT_INFO) * (objectCount - 1);
	std::shared_ptr<std::vector<byte>> sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_ENTER_ZONE* send = reinterpret_cast<PACKET_S2C_ENTER_ZONE*>(sendBuffer->data());
	send->proxyHeader = *proxy;
	send->packetHeader = {static_cast<uint16_t>(packetSize - sizeof(PROXY_HEADER)), ePacketType::S2C_EnterZone};
	send->result = eZoneResult::Success;
	send->channelId = channel->GetChannelId();
	send->playerObjectNum = player->GetNum();
	send->experience = player->GetExperience();
	send->requiredExperience = player->GetRequiredExperience();
	send->objectCount = objectCount;
	FillObjectInfo(player, send->objectList[0]);
	uint64_t index = 1;
	std::set<CFieldObject*>::iterator iter = player->GetVisibleObjectList().begin();
	std::set<CFieldObject*>::iterator end = player->GetVisibleObjectList().end();
	for (iter; iter != end; ++iter)
	{
		FillObjectInfo(*iter, send->objectList[index]);
		++index;
	}
	m_owner->PostSend(sendBuffer);
	for (iter = player->GetVisibleObjectList().begin(); iter != end; ++iter)
	{
		SendObjectMove(session, *iter, m_owner);
	}
	player->ClearViewEventList();
	FlushViewEventList(channel);
}

void CPacketTask::OnLeaveZone()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return;
	CPlayer* player = session->GetPlayer();
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (player == nullptr || channel == nullptr) return;
	PLAYER_DATA data{};
	data.accountIndex = proxy->accountIndex;
	data.characterIndex = player->GetCharacterIndex();
	data.jobClass = static_cast<eJobClass>(player->GetClass());
	data.level = player->GetLevel();
	data.experience = player->GetExperience();
	data.zoneId = m_zoneId;
	data.position = {player->GetPosition().x, 0.0f, player->GetPosition().z};
	if (player->GetState() == ePlayerState::Die)
	{
		ZONE_INFO town{};
		if (m_zoneData->FindZone(TOWN_ZONE_ID, town) && town.isActive)
		{
			data.zoneId = town.zoneId;
			data.position = town.enterPosition;
		}
	}
	m_characterDB->Save(data);
	channel->RemovePlayer(player);
	session->ClearPlayer();
	FlushViewEventList(channel);
	m_sessionManager->Deactivate(proxy->sessionId);

	PACKET_S2C_LEAVE_ZONE send;
	send.proxyHeader = *proxy;
	send.result = eZoneResult::Success;
	m_owner->PostSend(MakeShared(send));
}

void CPacketTask::OnMove()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return;

	CPlayer* player = session->GetPlayer();
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (player == nullptr || channel == nullptr) return;
	PACKET_C2S_MOVE* recv = reinterpret_cast<PACKET_C2S_MOVE*>(m_packet.data() + sizeof(PROXY_HEADER));
	VECTOR3 position{recv->position.x, 0.0f, recv->position.z};
	if (!channel->MovePlayer(player, position)) return;
	player->UpdateRotationY(recv->rotationY);
	// 정기 위치 패킷에서도 목적지 갱신
	player->UpdateDestPosition({recv->destination.x, 0.0f, recv->destination.z});
	if (player->GetAttackType() == eAttackType::None)
	{
		if (VECTOR3::Distance(player->GetPosition(), player->GetDestPosition()) > 0.01f)
			player->UpdateState(ePlayerState::Move);
		else
			player->UpdateState(ePlayerState::Idle);
	}
	FlushViewEventList(channel);
	BroadcastMove(channel, player);
}

void CPacketTask::OnAttackStart()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return;
	CPlayer* player = session->GetPlayer();
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (player == nullptr || channel == nullptr) return;
	PACKET_C2S_ATTACK_START* recv = reinterpret_cast<PACKET_C2S_ATTACK_START*>(m_packet.data() + sizeof(PROXY_HEADER));
	VECTOR3 position{recv->position.x, 0.0f, recv->position.z};
	if (!channel->MovePlayer(player, position)) return;
	if (recv->targetType != eZoneObjectType::Monster) return;
	if (recv->attackType == eAttackType::None) return;
	player->UpdateRotationY(recv->rotationY);
	player->UpdateDestPosition(position);
	FlushViewEventList(channel);
	if (player->StartAttack(recv->attackType, recv->targetObjectNum)) BroadcastAttackStart(channel, player);
}

void CPacketTask::OnAttackExecute()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return;
	CPlayer* player = session->GetPlayer();
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (player == nullptr || channel == nullptr) return;
	PACKET_C2S_ATTACK_EXECUTE* recv = reinterpret_cast<PACKET_C2S_ATTACK_EXECUTE*>(m_packet.data() + sizeof(PROXY_HEADER));
	VECTOR3 position{recv->position.x, 0.0f, recv->position.z};
	PACKET_S2C_ATTACK_RESULT send;
	send.proxyHeader = *proxy;
	send.playerObjectNum = player->GetNum();
	if (!channel->MovePlayer(player, position))
	{
		m_owner->PostSend(MakeShared(send));
		player->ClearAttack();
		return;
	}
	player->UpdateRotationY(recv->rotationY);
	eAttackType attackType = player->GetAttackType();
	FlushViewEventList(channel);
	uint64_t beforeLevel = player->GetLevel();
	if (attackType == eAttackType::RapidFire && !player->ConsumeRapidHit(recv->hitIndex))
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	CMonster* monster = channel->GetMonsterManager()->Find(player->GetAttackTargetObjectNum());
	// 클라이언트 표면 사거리 1 + 플레이어/몬스터 히트박스 반경을 포함
	float attackRange = 2.5f;
	if (player->GetClass() == eClassState::Archer) attackRange = 12.0f;
	if (player->GetClass() == eClassState::Warrior && attackType == eAttackType::SkillS)
	{
		// 범위 공격 시작 시점의 레벨 사용
		uint64_t damage = CCombatDamageCalculator::CalculatePlayerDamage(player->GetClass(), beforeLevel, attackType);
		std::vector<std::unique_ptr<CMonster>>& monsterList = channel->GetMonsterManager()->GetList();
		uint64_t size = static_cast<uint64_t>(monsterList.size());
		bool sent = false;
		for (uint64_t i = 0; i < size; ++i)
		{
			CMonster* areaMonster = monsterList[i].get();
			if (!player->IsVisible(areaMonster)) continue;
			if (VECTOR3::Distance(player->GetPosition(), areaMonster->GetPosition()) > 6.0f) continue;
			uint64_t beforeHP = areaMonster->GetHP();
			send.gainedExperience = areaMonster->TakeDamage(player, damage, GetTickCount64());
			channel->NotifyMonsterUpdated(areaMonster);
			send.result = eZoneResult::Success;
			send.targetType = eZoneObjectType::Monster;
			send.targetObjectNum = areaMonster->GetNum();
			send.targetHP = areaMonster->GetHP();
			send.damage = beforeHP - send.targetHP;
			BroadcastAttackResult(channel, player, areaMonster, send);
			sent = true;
		}
		if (!sent)
		{
			// 빈 범위기 응답을 0번 몬스터 HP=0으로 해석하면 안됨
			send.result = eZoneResult::Success;
			send.targetObjectNum = UINT64_MAX;
			m_owner->PostSend(MakeShared(send));
		}
		if (player->GetLevel() != beforeLevel) BroadcastLevelUp(channel, player);
		player->ClearAttack();
		return;
	}
	if (monster == nullptr || attackType == eAttackType::None || !player->IsVisible(monster))
	{
		m_owner->PostSend(MakeShared(send));
		player->ClearAttack();
		return;
	}
	if (VECTOR3::Distance(player->GetPosition(), monster->GetPosition()) > attackRange)
	{
		m_owner->PostSend(MakeShared(send));
		player->ClearAttack();
		return;
	}
	if (!channel->GetField()->IsClearLine(player->GetPosition(), monster->GetPosition()))
	{
		m_owner->PostSend(MakeShared(send));
		player->ClearAttack();
		return;
	}
	uint64_t damage = CCombatDamageCalculator::CalculatePlayerDamage(player->GetClass(), player->GetLevel(), attackType);
	uint64_t beforeHP = monster->GetHP();
	send.gainedExperience = monster->TakeDamage(player, damage, GetTickCount64());
	channel->NotifyMonsterUpdated(monster);
	send.result = eZoneResult::Success;
	send.targetType = eZoneObjectType::Monster;
	send.targetObjectNum = monster->GetNum();
	send.targetHP = monster->GetHP();
	send.damage = beforeHP - send.targetHP;
	BroadcastAttackResult(channel, player, monster, send);
	if (player->GetLevel() != beforeLevel) BroadcastLevelUp(channel, player);
	if (attackType != eAttackType::RapidFire || recv->hitIndex >= 19 || monster->GetHP() == 0) player->ClearAttack();
}

void CPacketTask::OnAttackCancel()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return;
	CPlayer* player = session->GetPlayer();
	if (player == nullptr) return;
	player->ClearAttack();
	// None으로 공격 취소 통지
	BroadcastAttackStart(m_channelManager->FindChannel(session->GetChannelId()), player);
}

void CPacketTask::OnUsePotion()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return;
	CPlayer* player = session->GetPlayer();
	if (player == nullptr) return;

	PACKET_S2C_USE_POTION send;
	send.proxyHeader = *proxy;
	send.playerObjectNum = player->GetNum();
	send.maxHP = player->GetMaxHP();
	send.recoveryHP = player->RecoverHalfHP();
	send.currentHP = player->GetHP();
	if (send.recoveryHP > 0) send.result = eZoneResult::Success;
	m_owner->PostSend(MakeShared(send));
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (channel == nullptr || send.recoveryHP == 0) return;
	std::vector<CSession*> sessionList = m_sessionManager->GetActiveSessionList();
	for (uint64_t i = 0; i < sessionList.size(); ++i)
	{
		CSession* observerSession = sessionList[i];
		if (observerSession == session || observerSession->GetChannelId() != session->GetChannelId()) continue;
		CPlayer* observer = observerSession->GetPlayer();
		if (observer == nullptr || !observer->IsVisible(player)) continue;
		send.proxyHeader = {observerSession->GetSessionId(), observerSession->GetAccountIndex()};
		m_owner->PostSend(MakeShared(send));
	}
}

void CPacketTask::OnChat()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* senderSession = m_sessionManager->Find(proxy->sessionId);
	if (senderSession == nullptr) return;
	CPlayer* sender = senderSession->GetPlayer();
	if (sender == nullptr) return;
	PACKET_C2S_CHAT* recv = reinterpret_cast<PACKET_C2S_CHAT*>(m_packet.data() + sizeof(PROXY_HEADER));
	uint8_t nameSize = static_cast<uint8_t>(sender->GetName().size());
	uint64_t packetSize = sizeof(PACKET_S2C_CHAT) - 1 + nameSize + recv->chatSize;
	if (packetSize - sizeof(PROXY_HEADER) > UINT16_MAX) return;

	std::vector<CSession*> sessionList = m_sessionManager->GetActiveSessionList();
	uint64_t sessionSize = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < sessionSize; ++i)
	{
		CSession* receiverSession = sessionList[i];
		if (receiverSession->GetChannelId() != senderSession->GetChannelId()) continue;
		if (receiverSession->GetPlayer() == nullptr) continue;
		std::shared_ptr<std::vector<byte>> sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
		PACKET_S2C_CHAT* send = reinterpret_cast<PACKET_S2C_CHAT*>(sendBuffer->data());
		send->proxyHeader = {receiverSession->GetSessionId(), receiverSession->GetAccountIndex()};
		send->packetHeader = {static_cast<uint16_t>(packetSize - sizeof(PROXY_HEADER)), ePacketType::S2C_Chat};
		send->nameSize = nameSize;
		send->chatSize = recv->chatSize;
		if (nameSize > 0) memcpy(send->text, sender->GetName().data(), nameSize);
		if (recv->chatSize > 0) memcpy(send->text + nameSize, recv->text, recv->chatSize);
		m_owner->PostSend(sendBuffer);
	}
}

void CPacketTask::OnChangeZone()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_C2S_CHANGE_ZONE* recv = reinterpret_cast<PACKET_C2S_CHANGE_ZONE*>(m_packet.data() + sizeof(PROXY_HEADER));
	PACKET_S2C_CHANGE_ZONE send;
	send.proxyHeader = *proxy;
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr || session->GetPlayer() == nullptr)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (channel == nullptr)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	if (session->GetPlayer()->GetState() == ePlayerState::Die)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	ZONE_PORTAL_INFO portalInfo{};
	if (!m_zoneData->FindPortal(m_zoneId, recv->portalId, portalInfo) || !portalInfo.isActive)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	ZONE_INFO targetZone{};
	if (!m_zoneData->FindZone(portalInfo.targetZoneId, targetZone) || !targetZone.isActive)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	CPlayer* player = session->GetPlayer();
	PLAYER_DATA data{};
	data.accountIndex = session->GetAccountIndex();
	data.characterIndex = player->GetCharacterIndex();
	data.jobClass = static_cast<eJobClass>(player->GetClass());
	data.level = player->GetLevel();
	data.experience = player->GetExperience();
	data.zoneId = portalInfo.targetZoneId;
	data.position = portalInfo.enterPosition;
	data.position.y = 0.0f;
	if (!m_characterDB->Save(data))
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	channel->RemovePlayer(player);
	session->ClearPlayer();
	FlushViewEventList(channel);
	m_sessionManager->Deactivate(proxy->sessionId);
	send.result = eZoneResult::Success;
	send.zoneId = portalInfo.targetZoneId;
	send.position = portalInfo.enterPosition;
	send.position.y = 0.0f;
	m_owner->PostSend(MakeShared(send));
}

void CPacketTask::OnRespawn()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_S2C_RESPAWN send;
	send.proxyHeader = *proxy;
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr || session->GetPlayer() == nullptr)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	CPlayer* player = session->GetPlayer();
	CChannel* channel = m_channelManager->FindChannel(session->GetChannelId());
	if (channel == nullptr || player->GetState() != ePlayerState::Die)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}

	// 포탈 입장 위치가 아닌 마을의 기본 생성 위치를 사용
	ZONE_INFO town{};
	if (!m_zoneData->FindZone(TOWN_ZONE_ID, town) || !town.isActive)
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}
	PLAYER_DATA data{};
	data.accountIndex = session->GetAccountIndex();
	data.characterIndex = player->GetCharacterIndex();
	data.jobClass = static_cast<eJobClass>(player->GetClass());
	data.level = player->GetLevel();
	data.experience = player->GetExperience();
	data.zoneId = town.zoneId;
	data.position = town.enterPosition;
	data.position.y = 0.0f;
	if (!m_characterDB->Save(data))
	{
		m_owner->PostSend(MakeShared(send));
		return;
	}

	// 저장에 실패하면 시체/세션을 유지해서 다시 요청할 수 있게 함
	channel->RemovePlayer(player);
	session->ClearPlayer();
	FlushViewEventList(channel);
	m_sessionManager->Deactivate(proxy->sessionId);
	send.result = eZoneResult::Success;
	send.zoneId = town.zoneId;
	send.position = data.position;
	m_owner->PostSend(MakeShared(send));
}

void CPacketTask::OnChannelList()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr || session->GetPlayer() == nullptr) return;
	uint64_t channelCount = m_channelManager->GetChannelCount();
	if (channelCount == 0) return;
	uint64_t packetSize = sizeof(PACKET_S2C_CHANNEL_LIST) + sizeof(CHANNEL_INFO) * (channelCount - 1);
	if (packetSize - sizeof(PROXY_HEADER) > UINT16_MAX) return;
	std::shared_ptr<std::vector<byte>> sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_CHANNEL_LIST* send = reinterpret_cast<PACKET_S2C_CHANNEL_LIST*>(sendBuffer->data());
	send->proxyHeader = *proxy;
	send->packetHeader = {static_cast<uint16_t>(packetSize - sizeof(PROXY_HEADER)), ePacketType::S2C_ChannelList};
	send->channelCount = channelCount;
	for (uint64_t i = 0; i < channelCount; ++i)
	{
		CChannel* channel = m_channelManager->FindChannel(i + 1);
		send->channelList[i].channelId = i + 1;
		send->channelList[i].playerCount = channel->GetPlayerCount();
		send->channelList[i].maxPlayerCount = channel->GetMaxPlayerCount();
		send->channelList[i].available = channel->GetPlayerCount() < channel->GetMaxPlayerCount();
	}
	m_owner->PostSend(sendBuffer);
}

bool CPacketTask::OnChangeChannel()
{
	PROXY_HEADER* proxy = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(proxy->sessionId);
	if (session == nullptr) return false;
	if (m_channelTransferEnter)
	{
		CPlayer* player = m_channel->CreateReservedPlayer(m_transferData.characterIndex, m_transferName,
			static_cast<eClassState>(m_transferData.jobClass),
			{m_transferData.position.x, 0.0f, m_transferData.position.z},
			m_transferData.level, m_transferData.experience);
		if (player == nullptr)
		{
			SendChangeChannel(session, m_channel, nullptr, eZoneResult::Fail);
			return false;
		}
		// 채널만 옮겼다고 풀피가 되면 안됨
		if (m_transferData.hp < player->GetMaxHP()) player->TakeDamage(player->GetMaxHP() - m_transferData.hp);
		player->UpdateRotationY(m_transferData.rotationY);
		session->SetPlayer(m_transferData.characterIndex, m_channel->GetChannelId(), player);
		SendChangeChannel(session, m_channel, player, eZoneResult::Success);
		player->ClearViewEventList();
		FlushViewEventList(m_channel);
		return false;
	}
	CPlayer* player = session->GetPlayer();
	CChannel* oldChannel = m_channelManager->FindChannel(session->GetChannelId());
	PACKET_C2S_CHANGE_CHANNEL* recv = reinterpret_cast<PACKET_C2S_CHANGE_CHANNEL*>(m_packet.data() + sizeof(PROXY_HEADER));
	CChannel* targetChannel = m_channelManager->FindChannel(recv->channelId);
	if (player == nullptr || oldChannel == nullptr || targetChannel == nullptr)
	{
		SendChangeChannel(session, targetChannel, nullptr, eZoneResult::Fail);
		return false;
	}
	if (oldChannel == targetChannel)
	{
		SendChangeChannel(session, oldChannel, player, eZoneResult::Success);
		return false;
	}
	if (player->GetState() == ePlayerState::Die)
	{
		SendChangeChannel(session, targetChannel, nullptr, eZoneResult::Fail);
		return false;
	}
	if (!targetChannel->ReservePlayer())
	{
		SendChangeChannel(session, targetChannel, nullptr, eZoneResult::Full);
		return false;
	}
	m_transferData.characterIndex = player->GetCharacterIndex();
	m_transferName = player->GetName();
	m_transferData.accountIndex = session->GetAccountIndex();
	m_transferData.jobClass = static_cast<eJobClass>(player->GetClass());
	m_transferData.level = player->GetLevel();
	m_transferData.experience = player->GetExperience();
	m_transferData.maxHP = player->GetMaxHP();
	m_transferData.hp = player->GetHP();
	m_transferData.rotationY = player->GetRotationY();
	m_transferData.zoneId = m_zoneId;
	m_transferData.position = {player->GetPosition().x, 0.0f, player->GetPosition().z};
	oldChannel->RemovePlayer(player);
	session->ClearPlayer();
	FlushViewEventList(oldChannel);
	m_channel = targetChannel;
	m_channelTransferEnter = true;
	targetChannel->PushJob(this);
	return true;
}

void CPacketTask::FillObjectInfo(CFieldObject* _object, FIELD_OBJECT_INFO& _info)
{
	_info = {};
	if (_object == nullptr) return;
	_info.objectNum = _object->GetNum();
	_info.position = {_object->GetPosition().x, 0.0f, _object->GetPosition().z};
	_info.rotationY = _object->GetRotationY();
	if (_object->GetObjectType() == eFieldObjectType::Player)
	{
		CPlayer* player = static_cast<CPlayer*>(_object);
		_info.objectType = eZoneObjectType::Player;
		_info.dataIndex = player->GetCharacterIndex();
		_info.jobClass = static_cast<eJobClass>(player->GetClass());
		_info.level = player->GetLevel();
		_info.maxHP = player->GetMaxHP();
		_info.hp = player->GetHP();
		_info.state = eZoneObjectState::Idle;
		_info.nameSize = static_cast<uint8_t>(player->GetName().size());
		if (_info.nameSize > 30) _info.nameSize = 30;
		if (_info.nameSize > 0) memcpy(_info.name, player->GetName().data(), _info.nameSize);
		if (player->GetState() == ePlayerState::Move) _info.state = eZoneObjectState::Move;
		if (player->GetState() == ePlayerState::Attack) _info.state = eZoneObjectState::Attack;
		if (player->GetState() == ePlayerState::Die) _info.state = eZoneObjectState::Dead;
		return;
	}
	CMonster* monster = static_cast<CMonster*>(_object);
	_info.objectType = eZoneObjectType::Monster;
	_info.dataIndex = static_cast<uint64_t>(monster->GetType());
	_info.level = monster->GetLevel();
	_info.maxHP = monster->GetMaxHP();
	_info.hp = monster->GetHP();
	_info.state = static_cast<eZoneObjectState>(monster->GetState());
}

void CPacketTask::FlushViewEventList(CChannel* _channel)
{
	FlushViewEventList(_channel, m_sessionManager, m_owner);
}

void CPacketTask::FlushViewEventList(CChannel* _channel, CSessionManager* _sessionManager, CTCPConnection* _owner)
{
	if (_channel == nullptr || _sessionManager == nullptr || _owner == nullptr) return;
	std::vector<CSession*> sessionList = _sessionManager->GetActiveSessionList();
	uint64_t sessionSize = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < sessionSize; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* player = session->GetPlayer();
		if (player == nullptr) continue;
		std::vector<FIELD_VIEW_EVENT>& eventList = player->GetViewEventList();
		uint64_t eventSize = static_cast<uint64_t>(eventList.size());
		for (uint64_t j = 0; j < eventSize; ++j)
		{
			PROXY_HEADER proxyHeader{session->GetSessionId(), session->GetAccountIndex()};
			FIELD_VIEW_EVENT& event = eventList[j];
			if (event.type == eFieldViewEventType::Leave)
			{
				PACKET_S2C_FIELD_OBJECT_LEAVE send;
				send.proxyHeader = proxyHeader;
				send.objectNum = event.objectNum;
				if (event.objectType == eFieldObjectType::Monster) send.objectType = eZoneObjectType::Monster;
				_owner->PostSend(MakeShared(send));
				continue;
			}
			CFieldObject* object = nullptr;
			if (event.objectType == eFieldObjectType::Player) object = _channel->GetPlayerManager()->Find(event.objectNum);
			if (event.objectType == eFieldObjectType::Monster) object = _channel->GetMonsterManager()->Find(event.objectNum);
			if (object == nullptr) continue;
			PACKET_S2C_FIELD_OBJECT_ENTER send;
			send.proxyHeader = proxyHeader;
			FillObjectInfo(object, send.objectInfo);
			_owner->PostSend(MakeShared(send));
			SendObjectMove(session, object, _owner);
		}
		player->ClearViewEventList();
	}
}

void CPacketTask::BroadcastMonsterState(CChannel* _channel, CSessionManager* _sessionManager, CTCPConnection* _owner, CMonster* _monster)
{
	if (_channel == nullptr || _sessionManager == nullptr || _owner == nullptr || _monster == nullptr) return;
	FlushViewEventList(_channel, _sessionManager, _owner);
	CPlayer* target = _channel->GetPlayerManager()->Find(_monster->GetAttackTargetObjectNum());
	// 피격 통지는 한번만 전송
	uint64_t damage = _monster->ConsumeAttackDamage();
	bool pathChanged = _monster->ConsumePathChanged();
	std::vector<CSession*> sessionList = _sessionManager->GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* observer = session->GetPlayer();
		if (observer == nullptr) continue;
		if (observer->IsVisible(_monster))
		{
			PACKET_S2C_MONSTER_STATE send;
			send.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
			send.monsterObjectNum = _monster->GetNum();
			send.state = static_cast<eZoneObjectState>(_monster->GetState());
			send.position = {_monster->GetPosition().x, 0.0f, _monster->GetPosition().z};
			send.destination = {_monster->GetDestination().x, 0.0f, _monster->GetDestination().z};
			send.rotationY = _monster->GetRotationY();
			send.hp = _monster->GetHP();
			if (target != nullptr) send.targetPlayerObjectNum = target->GetNum();
			_owner->PostSend(MakeShared(send));
			if (pathChanged && (_monster->GetState() == eMonsterState::Move || _monster->GetState() == eMonsterState::Chase || _monster->GetState() == eMonsterState::Return))
				SendMonsterMove(session, _monster, _owner);
		}
		// 피격된 플레이어가 보이는 경우에도 전송
		if (damage == 0 || target == nullptr) continue;
		if (observer != target && !observer->IsVisible(target)) continue;
		PACKET_S2C_PLAYER_HIT hit;
		hit.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
		hit.playerObjectNum = target->GetNum();
		hit.monsterObjectNum = _monster->GetNum();
		hit.damage = damage;
		hit.hp = target->GetHP();
		_owner->PostSend(MakeShared(hit));
	}
	if (damage > 0 && target != nullptr && target->GetState() == ePlayerState::Die)
		BroadcastPlayerDead(_channel, _sessionManager, _owner, target);
}

void CPacketTask::BroadcastPlayerDead(CChannel* _channel, CSessionManager* _sessionManager, CTCPConnection* _owner, CPlayer* _player)
{
	if (_channel == nullptr || _sessionManager == nullptr || _owner == nullptr || _player == nullptr) return;
	if (_player->GetState() != ePlayerState::Die) return;
	std::vector<CSession*> sessionList = _sessionManager->GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* observer = session->GetPlayer();
		if (observer == nullptr) continue;
		if (observer != _player && !observer->IsVisible(_player)) continue;
		PACKET_S2C_PLAYER_DEAD send;
		send.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
		send.playerObjectNum = _player->GetNum();
		send.position = {_player->GetPosition().x, 0.0f, _player->GetPosition().z};
		send.rotationY = _player->GetRotationY();
		_owner->PostSend(MakeShared(send));
	}
}

void CPacketTask::SendObjectMove(CSession* _session, CFieldObject* _object, CTCPConnection* _owner)
{
	if (_session == nullptr || _object == nullptr || _owner == nullptr) return;
	// 최초 입장, 채널 변경, 섹터 진입 모두 같은 이동 스냅샷 사용
	if (_object->GetObjectType() == eFieldObjectType::Monster)
	{
		CMonster* monster = static_cast<CMonster*>(_object);
		if (monster->GetState() == eMonsterState::Move || monster->GetState() == eMonsterState::Chase || monster->GetState() == eMonsterState::Return)
			SendMonsterMove(_session, monster, _owner);
		return;
	}
	CPlayer* player = static_cast<CPlayer*>(_object);
	if (player->GetState() != ePlayerState::Move) return;
	PACKET_S2C_MOVE send;
	send.proxyHeader = {_session->GetSessionId(), _session->GetAccountIndex()};
	send.playerObjectNum = player->GetNum();
	send.position = {player->GetPosition().x, 0.0f, player->GetPosition().z};
	send.destination = {player->GetDestPosition().x, 0.0f, player->GetDestPosition().z};
	send.rotationY = player->GetRotationY();
	_owner->PostSend(MakeShared(send));
}

void CPacketTask::SendMonsterMove(CSession* _session, CMonster* _monster, CTCPConnection* _owner)
{
	if (_session == nullptr || _monster == nullptr || _owner == nullptr) return;
	std::vector<VECTOR3>& path = _monster->GetPath();
	uint64_t pathCount = static_cast<uint64_t>(path.size());
	uint64_t packetSize = sizeof(PACKET_S2C_MONSTER_MOVE) - sizeof(ZONE_VECTOR3) + sizeof(ZONE_VECTOR3) * pathCount;
	if (packetSize - sizeof(PROXY_HEADER) > UINT16_MAX) return;
	std::shared_ptr<std::vector<byte>> sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_MONSTER_MOVE* send = reinterpret_cast<PACKET_S2C_MONSTER_MOVE*>(sendBuffer->data());
	send->proxyHeader = {_session->GetSessionId(), _session->GetAccountIndex()};
	send->packetHeader = {static_cast<uint16_t>(packetSize - sizeof(PROXY_HEADER)), ePacketType::S2C_MonsterMove};
	send->monsterObjectNum = _monster->GetNum();
	send->position = {_monster->GetPosition().x, 0.0f, _monster->GetPosition().z};
	send->rotationY = _monster->GetRotationY();
	send->moveSpeed = _monster->GetMoveSpeed();
	send->pathIndex = _monster->GetPathIndex();
	send->pathCount = pathCount;
	for (uint64_t i = 0; i < pathCount; ++i)
		send->path[i] = {path[i].x, 0.0f, path[i].z};
	_owner->PostSend(sendBuffer);
}

void CPacketTask::BroadcastMove(CChannel* _channel, CPlayer* _player)
{
	if (_channel == nullptr || _player == nullptr) return;
	std::vector<CSession*> sessionList = m_sessionManager->GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* observer = session->GetPlayer();
		if (observer == nullptr || observer == _player || !observer->IsVisible(_player)) continue;
		PACKET_S2C_MOVE send;
		send.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
		send.playerObjectNum = _player->GetNum();
		send.position = {_player->GetPosition().x, 0.0f, _player->GetPosition().z};
		send.destination = {_player->GetDestPosition().x, 0.0f, _player->GetDestPosition().z};
		send.rotationY = _player->GetRotationY();
		m_owner->PostSend(MakeShared(send));
	}
}

void CPacketTask::BroadcastAttackStart(CChannel* _channel, CPlayer* _player)
{
	if (_channel == nullptr || _player == nullptr) return;
	std::vector<CSession*> sessionList = m_sessionManager->GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* observer = session->GetPlayer();
		if (observer == nullptr || observer == _player || !observer->IsVisible(_player)) continue;
		PACKET_S2C_ATTACK_START send;
		send.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
		send.playerObjectNum = _player->GetNum();
		send.attackType = _player->GetAttackType();
		send.targetObjectNum = _player->GetAttackTargetObjectNum();
		send.position = {_player->GetPosition().x, 0.0f, _player->GetPosition().z};
		send.rotationY = _player->GetRotationY();
		m_owner->PostSend(MakeShared(send));
	}
}

void CPacketTask::BroadcastAttackResult(CChannel* _channel, CPlayer* _player, CMonster* _monster, const PACKET_S2C_ATTACK_RESULT& _packet)
{
	if (_channel == nullptr || _player == nullptr) return;
	std::vector<CSession*> sessionList = m_sessionManager->GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* observer = session->GetPlayer();
		if (observer == nullptr) continue;
		if (observer != _player && !observer->IsVisible(_player))
		{
			if (_monster == nullptr || !observer->IsVisible(_monster)) continue;
		}
		PACKET_S2C_ATTACK_RESULT send = _packet;
		send.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
		if (observer != _player) send.gainedExperience = 0;
		m_owner->PostSend(MakeShared(send));
	}
}

void CPacketTask::BroadcastLevelUp(CChannel* _channel, CPlayer* _player)
{
	if (_channel == nullptr || _player == nullptr) return;
	std::vector<CSession*> sessionList = m_sessionManager->GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		if (session->GetChannelId() != _channel->GetChannelId()) continue;
		CPlayer* observer = session->GetPlayer();
		if (observer == nullptr) continue;
		if (observer != _player && !observer->IsVisible(_player)) continue;

		PACKET_S2C_LEVEL_UP send;
		send.proxyHeader = {session->GetSessionId(), session->GetAccountIndex()};
		send.playerObjectNum = _player->GetNum();
		send.level = _player->GetLevel();
		send.experience = _player->GetExperience();
		send.requiredExperience = _player->GetRequiredExperience();
		send.maxHP = _player->GetMaxHP();
		send.hp = _player->GetHP();
		m_owner->PostSend(MakeShared(send));
	}
}

void CPacketTask::SendEnterFail(PROXY_HEADER* _proxyHeader, eZoneResult _result)
{
	if (_proxyHeader == nullptr) return;
	PACKET_S2C_ENTER_ZONE send{};
	send.proxyHeader = *_proxyHeader;
	send.packetHeader = {static_cast<uint16_t>(sizeof(send) - sizeof(PROXY_HEADER)), ePacketType::S2C_EnterZone};
	send.result = _result;
	send.channelId = 0;
	send.playerObjectNum = 0;
	send.experience = 0;
	send.requiredExperience = 0;
	send.objectCount = 0;
	m_owner->PostSend(MakeShared(send));
}

void CPacketTask::SendChangeChannel(CSession* _session, CChannel* _channel, CPlayer* _player, eZoneResult _result)
{
	if (_session == nullptr) return;
	uint64_t objectCount = 0;
	if (_player != nullptr) objectCount = static_cast<uint64_t>(_player->GetVisibleObjectList().size()) + 1;
	uint64_t packetSize = sizeof(PACKET_S2C_CHANGE_CHANNEL);
	if (objectCount > 1) packetSize += sizeof(FIELD_OBJECT_INFO) * (objectCount - 1);
	std::shared_ptr<std::vector<byte>> sendBuffer = std::make_shared<std::vector<byte>>(packetSize);
	PACKET_S2C_CHANGE_CHANNEL* send = reinterpret_cast<PACKET_S2C_CHANGE_CHANNEL*>(sendBuffer->data());
	send->proxyHeader = {_session->GetSessionId(), _session->GetAccountIndex()};
	send->packetHeader = {static_cast<uint16_t>(packetSize - sizeof(PROXY_HEADER)), ePacketType::S2C_ChangeChannel};
	send->result = _result;
	send->channelId = 0;
	if (_channel != nullptr) send->channelId = _channel->GetChannelId();
	send->playerObjectNum = 0;
	send->experience = 0;
	send->requiredExperience = 0;
	send->objectCount = objectCount;
	if (_player == nullptr)
	{
		m_owner->PostSend(sendBuffer);
		return;
	}
	send->playerObjectNum = _player->GetNum();
	send->experience = _player->GetExperience();
	send->requiredExperience = _player->GetRequiredExperience();
	FillObjectInfo(_player, send->objectList[0]);
	uint64_t index = 1;
	std::set<CFieldObject*>::iterator iter = _player->GetVisibleObjectList().begin();
	std::set<CFieldObject*>::iterator end = _player->GetVisibleObjectList().end();
	for (iter; iter != end; ++iter)
	{
		FillObjectInfo(*iter, send->objectList[index]);
		++index;
	}
	m_owner->PostSend(sendBuffer);
	for (iter = _player->GetVisibleObjectList().begin(); iter != end; ++iter)
	{
		SendObjectMove(_session, *iter, m_owner);
	}
}

bool CPacketTask::IsValidPacket(PACKET_HEADER* _header)
{
	if (_header == nullptr) return false;
	uint64_t packetSize = m_packet.size() - sizeof(PROXY_HEADER);
	if (_header->size != packetSize) return false;
	if (_header->type == ePacketType::C2S_EnterZone) return packetSize == sizeof(PACKET_C2S_ENTER_ZONE);
	if (_header->type == ePacketType::C2S_LeaveZone) return packetSize == sizeof(PACKET_C2S_LEAVE_ZONE);
	if (_header->type == ePacketType::C2S_Move) return packetSize == sizeof(PACKET_C2S_MOVE);
	if (_header->type == ePacketType::C2S_AttackStart) return packetSize == sizeof(PACKET_C2S_ATTACK_START);
	if (_header->type == ePacketType::C2S_AttackExecute) return packetSize == sizeof(PACKET_C2S_ATTACK_EXECUTE);
	if (_header->type == ePacketType::C2S_AttackCancel) return packetSize == sizeof(PACKET_C2S_ATTACK_CANCEL);
	if (_header->type == ePacketType::C2S_UsePotion) return packetSize == sizeof(PACKET_C2S_USE_POTION);
	if (_header->type == ePacketType::C2S_ChannelList) return packetSize == sizeof(PACKET_C2S_CHANNEL_LIST);
	if (_header->type == ePacketType::C2S_ChangeChannel) return packetSize == sizeof(PACKET_C2S_CHANGE_CHANNEL);
	if (_header->type == ePacketType::C2S_ChangeZone) return packetSize == sizeof(PACKET_C2S_CHANGE_ZONE);
	if (_header->type == ePacketType::C2S_Respawn) return packetSize == sizeof(PACKET_C2S_RESPAWN);
	if (_header->type == ePacketType::C2S_Chat)
	{
		if (packetSize < sizeof(PACKET_C2S_CHAT)) return false;
		PACKET_C2S_CHAT* packet = reinterpret_cast<PACKET_C2S_CHAT*>(_header);
		return packetSize == sizeof(PACKET_C2S_CHAT) + packet->chatSize - 1;
	}
	return false;
}

void CPacketTask::Cleanup()
{
	m_packet.clear();
	m_owner = nullptr;
	m_generation = 0;
	m_channel = nullptr;
	m_channelTransferEnter = false;
	m_transferData = {};
	m_transferName.clear();
	m_jobPool->Release(this);
}
