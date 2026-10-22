#include "PacketTask.h"
#include <cstddef>
#include <iostream>
#include "WorldPacketCreator.h"

CPacketTask::CPacketTask(CJobPool* _jobPool, CSessionManager* _sessionManager, CChatManager* _chatManager, CSessionController* _sessionController, CPartyManager* _partyManager) noexcept :
	m_jobPool(_jobPool), m_sessionManager(_sessionManager), m_chatManager(_chatManager), m_sessionController(_sessionController), m_partyManager(_partyManager)
{
}

void CPacketTask::Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner) noexcept
{
	m_packet = std::move(_packet);
	m_owner = _owner;
}

void CPacketTask::Execute()
{
	if (m_owner == nullptr)
	{
		Cleanup();
		return;
	}

	if (m_packet.empty())
	{
		if (m_partyManager != nullptr) m_partyManager->Cleanup();
		m_sessionManager->Cleanup();
		Cleanup();
		return;
	}

	if (!m_owner->IsConnected() || m_packet.size() < sizeof(PROXY_PACKET_HEADER))
	{
		Cleanup();
		return;
	}

	const PROXY_PACKET_HEADER& header = *reinterpret_cast<const PROXY_PACKET_HEADER*>(m_packet.data());
	if (header.packetHeader.size < sizeof(PACKET_HEADER) || sizeof(PROXY_HEADER) + header.packetHeader.size != m_packet.size())
	{
		Cleanup();
		return;
	}

	if (header.packetHeader.type == ePacketType::C2S_WorldChat) OnWorldChat();
	else if (header.packetHeader.type == ePacketType::C2S_Whisper) OnWhisper();
	else if (header.packetHeader.type == ePacketType::C2S_PartyChat) OnPartyChat();
	else if (header.packetHeader.type >= ePacketType::C2S_CreateParty && header.packetHeader.type <= ePacketType::S2S_PartyMemberStatus) OnParty();
	else if (header.packetHeader.type >= ePacketType::C2S_PartyList && header.packetHeader.type <= ePacketType::C2S_AnswerPartyApplication) OnParty();
	else // 나머지 세션 입장 이동 종료 패킷
	{
		CSession* session = m_sessionManager->Find(header.proxyHeader.sessionId);
		eSessionState state = eSessionState::SelectCharacter;
		uint64_t zone = 0;
		uint64_t channel = 0;
		uint64_t level = 0;
		if (session != nullptr)
		{
			state = session->GetState();
			zone = session->GetZoneId();
			channel = session->GetChannelId();
			level = session->GetPlayer().GetLevel();
		}
		m_sessionController->Process(m_packet);
		session = m_sessionManager->Find(header.proxyHeader.sessionId);
		if (m_partyManager != nullptr && session != nullptr && session->GetAccountIndex() == header.proxyHeader.accountIndex)
		{
			bool zoneChanged = state != session->GetState() || zone != session->GetZoneId() || channel != session->GetChannelId();
			if (zoneChanged || level != session->GetPlayer().GetLevel()) m_partyManager->RefreshSession(*session, zoneChanged);
		}
	}
	FlushPartyEvents();
	Cleanup();
}


void CPacketTask::Cleanup()
{
	m_packet.clear();
	m_owner = nullptr;
	m_jobPool->Release(this);
}


void CPacketTask::OnWorldChat()
{
	const uint64_t prefix = sizeof(PROXY_HEADER) + offsetof(PACKET_C2S_WORLD_CHAT, text);
	if (m_packet.size() < prefix) return;

	const PROXY_HEADER& header = *reinterpret_cast<const PROXY_HEADER*>(m_packet.data());
	const PACKET_C2S_WORLD_CHAT* packet = reinterpret_cast<const PACKET_C2S_WORLD_CHAT*>(m_packet.data() + sizeof(PROXY_HEADER));
	if (packet->chatSize == 0 || m_packet.size() != prefix + packet->chatSize) return;

	std::vector<PROXY_HEADER> clientList;
	const CSession* sender = m_chatManager->PrepareWorldChat(header.sessionId, header.accountIndex, clientList);
	if (sender == nullptr || clientList.empty()) return;

	const std::vector<byte> chat = CWorldPacketCreator::CreateWorldChat(sender->GetPlayer().GetName(), packet->text, packet->chatSize);
	const std::vector<std::shared_ptr<std::vector<byte>>> packetList = CWorldPacketCreator::CreateClientList(sender->GetHeader(), clientList, chat);
	for (uint64_t i = 0; i < packetList.size(); ++i)
	{
		if (!m_owner->PostSend(packetList[i]))
		{
			std::cerr << "World chat send failed\n";
			return;
		}
	}
}

void CPacketTask::OnWhisper()
{
	const uint64_t prefix = sizeof(PROXY_HEADER) + offsetof(PACKET_C2S_WHISPER, text);
	if (m_packet.size() < prefix) return;

	const PROXY_HEADER& header = *reinterpret_cast<const PROXY_HEADER*>(m_packet.data());
	const PACKET_C2S_WHISPER* packet = reinterpret_cast<const PACKET_C2S_WHISPER*>(m_packet.data() + sizeof(PROXY_HEADER));
	if (packet->nameSize == 0 || packet->nameSize > 30 || packet->chatSize == 0) return;
	if (m_packet.size() != prefix + packet->nameSize + packet->chatSize) return;

	const std::string name(packet->text, packet->nameSize);
	const CSession* sender = nullptr;
	const CSession* target = nullptr;
	eWhisperResult result = eWhisperResult::Fail;
	if (!m_chatManager->PrepareWhisper(header.sessionId, header.accountIndex, name, sender, target, result)) return;

	const char* text = packet->text + packet->nameSize;
	uint8_t chatSize = packet->chatSize;

	// 본인에게 보내면 결과만 한 번 전송
	if (result == eWhisperResult::Success && sender != target)
	{
		std::shared_ptr<std::vector<byte>> send = CWorldPacketCreator::CreateWhisper(target->GetHeader(), sender->GetPlayer().GetName(), text, chatSize);
		if (!m_owner->PostSend(send)) result = eWhisperResult::Fail;
	}
	if (result != eWhisperResult::Success)
	{
		text = nullptr;
		chatSize = 0;
	}

	std::shared_ptr<std::vector<byte>> send = CWorldPacketCreator::CreateWhisperResult(sender->GetHeader(), name, text, chatSize, result);
	if (!m_owner->PostSend(send)) std::cerr << "Whisper result send failed\n";
}

void CPacketTask::OnPartyChat()
{
	const uint64_t prefix = sizeof(PROXY_HEADER) + offsetof(PACKET_C2S_PARTY_CHAT, text);
	if (m_packet.size() < prefix) return;
	const PROXY_HEADER& header = *reinterpret_cast<const PROXY_HEADER*>(m_packet.data());
	const PACKET_C2S_PARTY_CHAT& packet = *reinterpret_cast<const PACKET_C2S_PARTY_CHAT*>(m_packet.data() + sizeof(PROXY_HEADER));
	if (packet.chatSize == 0 || m_packet.size() != prefix + packet.chatSize) return;
	std::vector<PROXY_HEADER> recipients;
	const CSession* sender = m_chatManager->PreparePartyChat(header.sessionId, header.accountIndex, packet.partyNum, recipients);
	if (sender == nullptr) return;
	const std::vector<byte> chat = CWorldPacketCreator::CreatePartyChat(packet.partyNum, sender->GetPlayer().GetName(), packet.text, packet.chatSize);
	const std::vector<std::shared_ptr<std::vector<byte>>> packets = CWorldPacketCreator::CreateClientList(sender->GetHeader(), recipients, chat);
	for (uint64_t i = 0; i < packets.size(); ++i) m_owner->PostSend(packets[i]);
}


void CPacketTask::OnParty()
{
	if (m_partyManager == nullptr) return;
	const PROXY_PACKET_HEADER& header = *reinterpret_cast<const PROXY_PACKET_HEADER*>(m_packet.data());
	CSession* session = m_sessionManager->Find(header.proxyHeader.sessionId);
	if (session == nullptr || session->GetAccountIndex() != header.proxyHeader.accountIndex) return;
	const ePacketType type = header.packetHeader.type;
	if (type == ePacketType::S2S_PartyMemberStatus)
	{
		if (m_packet.size() == sizeof(PACKET_S2S_PARTY_MEMBER_STATUS)) m_partyManager->UpdateStatus(*session, *reinterpret_cast<const PACKET_S2S_PARTY_MEMBER_STATUS*>(m_packet.data()));
		return;
	}
	const byte* data = m_packet.data() + sizeof(PROXY_HEADER);
	const uint64_t size = m_packet.size() - sizeof(PROXY_HEADER);
	ePartyResult result = ePartyResult::InvalidState;
	uint64_t number = session->GetPartyNum();
	if (type == ePacketType::C2S_CreateParty && size >= offsetof(PACKET_C2S_CREATE_PARTY, name))
	{
		const PACKET_C2S_CREATE_PARTY& packet = *reinterpret_cast<const PACKET_C2S_CREATE_PARTY*>(data);
		if (size != offsetof(PACKET_C2S_CREATE_PARTY, name) + packet.nameSize) return;
		result = m_partyManager->Create(*session, std::string(packet.name, packet.nameSize));
		number = 0;
	}
	else if (type == ePacketType::C2S_PartyList && size == sizeof(PACKET_C2S_PARTY_LIST))
	{
		const PACKET_C2S_PARTY_LIST& packet = *reinterpret_cast<const PACKET_C2S_PARTY_LIST*>(data);
		result = m_partyManager->GetPartyList(*session, packet.page);
		if (result == ePartyResult::Success) return;
		number = packet.page;
	}
	else if (type == ePacketType::C2S_ApplyParty && size == sizeof(PACKET_C2S_APPLY_PARTY))
	{
		const PACKET_C2S_APPLY_PARTY& packet = *reinterpret_cast<const PACKET_C2S_APPLY_PARTY*>(data);
		number = packet.partyNum;
		result = m_partyManager->Apply(*session, packet.partyNum, packet.createNum);
	}
	else if (type == ePacketType::C2S_CancelPartyApplication && size == sizeof(PACKET_C2S_CANCEL_PARTY_APPLICATION))
	{
		const PACKET_C2S_CANCEL_PARTY_APPLICATION& packet = *reinterpret_cast<const PACKET_C2S_CANCEL_PARTY_APPLICATION*>(data);
		number = packet.requestNum;
		result = m_partyManager->CancelApplication(*session, packet.requestNum);
	}
	else if (type == ePacketType::C2S_AnswerPartyApplication && size == sizeof(PACKET_C2S_ANSWER_PARTY_APPLICATION))
	{
		const PACKET_C2S_ANSWER_PARTY_APPLICATION& packet = *reinterpret_cast<const PACKET_C2S_ANSWER_PARTY_APPLICATION*>(data);
		if (packet.accept > 1) return;
		number = packet.requestNum;
		result = m_partyManager->AnswerApplication(*session, packet.requestNum, packet.accept != 0);
	}
	else if (type == ePacketType::C2S_InviteParty && size >= offsetof(PACKET_C2S_INVITE_PARTY, name))
	{
		const PACKET_C2S_INVITE_PARTY& packet = *reinterpret_cast<const PACKET_C2S_INVITE_PARTY*>(data);
		if (packet.nameSize == 0 || packet.nameSize > 30 || size != offsetof(PACKET_C2S_INVITE_PARTY, name) + packet.nameSize) return;
		number = packet.partyNum;
		result = m_partyManager->Invite(*session, packet.partyNum, std::string(packet.name, packet.nameSize));
	}
	else if (type == ePacketType::C2S_AnswerPartyInvite && size == sizeof(PACKET_C2S_ANSWER_PARTY_INVITE))
	{
		const PACKET_C2S_ANSWER_PARTY_INVITE& packet = *reinterpret_cast<const PACKET_C2S_ANSWER_PARTY_INVITE*>(data);
		if (packet.accept > 1) return;
		number = packet.inviteNum;
		result = m_partyManager->AnswerInvite(*session, packet.inviteNum, packet.accept != 0);
	}
	else if ((type == ePacketType::C2S_LeaveParty || type == ePacketType::C2S_DisbandParty) && size == sizeof(PACKET_C2S_PARTY))
	{
		const PACKET_C2S_PARTY& packet = *reinterpret_cast<const PACKET_C2S_PARTY*>(data);
		number = packet.partyNum;
		if (type == ePacketType::C2S_LeaveParty) result = m_partyManager->Leave(*session, packet.partyNum);
		else result = m_partyManager->Disband(*session, packet.partyNum);
	}
	else if ((type == ePacketType::C2S_KickParty || type == ePacketType::C2S_ChangePartyLeader) && size == sizeof(PACKET_C2S_PARTY_TARGET))
	{
		const PACKET_C2S_PARTY_TARGET& packet = *reinterpret_cast<const PACKET_C2S_PARTY_TARGET*>(data);
		number = packet.partyNum;
		if (type == ePacketType::C2S_KickParty) result = m_partyManager->Kick(*session, packet.partyNum, packet.sessionId, packet.characterIndex);
		else result = m_partyManager->ChangeLeader(*session, packet.partyNum, packet.sessionId, packet.characterIndex);
	}
	else return;
	m_owner->PostSend(CWorldPacketCreator::CreatePartyResult(session->GetHeader(), type, result, number));
}

void CPacketTask::FlushPartyEvents()
{
	if (m_partyManager == nullptr) return;
	const std::vector<std::shared_ptr<std::vector<byte>>> packets = CWorldPacketCreator::CreatePartyEvents(m_partyManager->TakeEvents());
	for (uint64_t i = 0; i < packets.size(); ++i)
	{
		if (!m_owner->PostSend(packets[i])) std::cerr << "Party event send failed\n";
	}
}
