#include "PacketTask.h"
#include "ClientConnection.h"
#include "../Utility_Core/JobPool.h"
#include "../Common/PACKET.h"
#include <cstddef>

CPacketTask::CPacketTask(CJobPool* _pool,
	CClientConnectionManager* _clientConnManager,
	CServerConnectionManager* _serverConnManager):
	m_owner(nullptr),
	m_generation(NULL),
	m_type(eOwnerType::None),
	m_jobPool(_pool),
	m_clientConnManager(_clientConnManager),
	m_serverConnManager(_serverConnManager)
{
}

void CPacketTask::Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation, eOwnerType _type)
{
	m_packet = std::move(_packet);
	m_owner = _owner;
	m_generation = _generation;
	m_type = _type;
}

void CPacketTask::Execute()
{
	if (m_owner == nullptr || m_owner->GetGeneration() != m_generation)
	{
		Cleanup();
		return;
	}
	if (m_type == eOwnerType::Client && m_packet.empty())
	{
		OnDisconnected();
		Cleanup();
		return;
	}
	if (m_type == eOwnerType::Client && !m_owner->IsConnected())
	{
		Cleanup();
		return;
	}
	if (m_type == eOwnerType::Client)
	{
		if (m_packet.size() < sizeof(PACKET_HEADER))
		{
			m_owner->Disconnect();
			Cleanup();
			return;
		}

		PACKET_HEADER* packetHeader = reinterpret_cast<PACKET_HEADER*>(m_packet.data());
		if (packetHeader->size != m_packet.size())
		{
			m_owner->Disconnect();
			Cleanup();
			return;
		}
	}
	if (m_type == eOwnerType::Server && m_packet.size() < sizeof(PROXY_HEADER) + sizeof(PACKET_HEADER))
	{
		Cleanup();
		return;
	}
	if (m_type == eOwnerType::Server)
	{
		PACKET_HEADER* packetHeader = reinterpret_cast<PACKET_HEADER*>(m_packet.data() + sizeof(PROXY_HEADER));
		if (static_cast<uint64_t>(packetHeader->size) + sizeof(PROXY_HEADER) != m_packet.size())
		{
			Cleanup();
			return;
		}
	}

	if (m_type == eOwnerType::Client) ExecuteC2S();
	if (m_type == eOwnerType::Server) ExecuteS2C();
	Cleanup();
}

void CPacketTask::OnDisconnected()
{
	CClientConnection* connection = static_cast<CClientConnection*>(m_owner);
	if (connection->IsLoginPending()) return;
	if (connection->GetAccountIndex() == 0)
	{
		m_clientConnManager->Release(connection);
		return;
	}
	if (!connection->TryStartLogout()) return;
	PACKET_S2S_LEAVE_WORLD send;
	send.proxyHeader = {connection->GetSessionId(), connection->GetAccountIndex()};
	CTCPConnection* world = m_serverConnManager->GetWorldConnection();
	if (world != nullptr) world->PostSend(MakeShared(send));
}

void CPacketTask::ExecuteC2S()
{
	CClientConnection* owner = static_cast<CClientConnection*>(m_owner);
	PACKET_HEADER* header = reinterpret_cast<PACKET_HEADER*>(m_packet.data());
	const ePacketType type = header->type;
	if (owner->GetAccountIndex() == 0 && type != ePacketType::C2S_Login && type != ePacketType::C2S_CreateAccount)
	{
		owner->Disconnect();
		return;
	}
	if (owner->GetAccountIndex() != 0 && (type == ePacketType::C2S_Login || type == ePacketType::C2S_CreateAccount))
	{
		owner->Disconnect();
		return;
	}
	// 서버 전용 패킷은 클라이언트가 보낼 수 없음.
	const uint16_t number = static_cast<uint16_t>(type);
	const bool partyRequest = (type >= ePacketType::C2S_CreateParty && type <= ePacketType::C2S_PartyChat) ||
		(type >= ePacketType::C2S_PartyList && type <= ePacketType::C2S_AnswerPartyApplication);
	if (number >= 3000 && type != ePacketType::C2S_WorldChat && type != ePacketType::C2S_Whisper && !partyRequest) return;
	if (type == ePacketType::C2S_Login && !owner->TryStartLogin()) return;
	if (type == ePacketType::C2S_Logout && !owner->TryStartLogout()) return;

	CTCPConnection* connection = nullptr;
	if (1000 <= number && number < 2000) connection = m_serverConnManager->GetLoginConnection();
	if (2000 <= number && number < 3000)
	{
		// 로딩 중에는 필드 요청만 막음. World 요청은 계속 전달.
		if (owner->GetZoneId() != 0) connection = m_serverConnManager->GetZoneConnection(owner->GetZoneId());
	}
	if (type == ePacketType::C2S_CharacterList || type == ePacketType::C2S_CreateCharacter ||
		type == ePacketType::C2S_SelectCharacter || type == ePacketType::C2S_EnterZone ||
		type == ePacketType::C2S_ChangeZone || type == ePacketType::C2S_Respawn ||
		type == ePacketType::C2S_Logout || type == ePacketType::C2S_LeaveZone || type == ePacketType::C2S_WorldChat || type == ePacketType::C2S_Whisper)
		connection = m_serverConnManager->GetWorldConnection();
	if (partyRequest) connection = m_serverConnManager->GetWorldConnection();

	if (connection == nullptr) return;
	std::shared_ptr<std::vector<byte>> relay = std::make_shared<std::vector<byte>>(AttachProxyHeader(m_packet, {owner->GetSessionId(), owner->GetAccountIndex()}));
	if (!connection->PostSend(relay))
	{
		if (type == ePacketType::C2S_Login) owner->SetLoginPending(false);
		owner->Disconnect();
	}
}

void CPacketTask::ExecuteS2C()
{
	PROXY_PACKET_HEADER* header = reinterpret_cast<PROXY_PACKET_HEADER*>(m_packet.data());
	const ePacketType type = header->packetHeader.type;
	CTCPConnection* world = m_serverConnManager->GetWorldConnection();
	CTCPConnection* login = m_serverConnManager->GetLoginConnection();
	CClientConnection* client = m_clientConnManager->Find(header->proxyHeader.sessionId);
	if (client == nullptr) return;

	if (m_owner == world && type == ePacketType::S2S_SendServer)
	{
		const uint64_t prefix = offsetof(PACKET_S2S_SEND_SERVER, packet);
		if (m_packet.size() < prefix + sizeof(PROXY_PACKET_HEADER)) return;
		PACKET_S2S_SEND_SERVER* relay = reinterpret_cast<PACKET_S2S_SEND_SERVER*>(m_packet.data());
		PROXY_PACKET_HEADER* inner = reinterpret_cast<PROXY_PACKET_HEADER*>(relay->packet);
		if (inner->packetHeader.size < sizeof(PACKET_HEADER) || prefix + sizeof(PROXY_HEADER) + inner->packetHeader.size != m_packet.size()) return;
		if (inner->proxyHeader.sessionId != header->proxyHeader.sessionId || inner->proxyHeader.accountIndex != header->proxyHeader.accountIndex) return;
		if (client->GetAccountIndex() != header->proxyHeader.accountIndex) return;
		CTCPConnection* target = nullptr;
		if (relay->serverType == eBackServerType::Login) target = login;
		if (relay->serverType == eBackServerType::Zone) target = m_serverConnManager->GetZoneConnection(relay->zoneId);
		std::shared_ptr<std::vector<byte>> packet = std::make_shared<std::vector<byte>>(relay->packet, relay->packet + m_packet.size() - prefix);
		if (target == nullptr || !target->PostSend(packet))
		{
			PACKET_S2S_ZONE fail(ePacketType::S2S_SendServerFail);
			fail.proxyHeader = header->proxyHeader;
			fail.zoneId = relay->zoneId;
			fail.portalId = static_cast<uint64_t>(inner->packetHeader.type);
			world->PostSend(MakeShared(fail));
		}
		return;
	}
	if (m_owner == world && type == ePacketType::S2S_SetZone)
	{
		if (m_packet.size() != sizeof(PACKET_S2S_SET_ZONE) || client->GetAccountIndex() != header->proxyHeader.accountIndex) return;
		client->SetZoneId(reinterpret_cast<PACKET_S2S_SET_ZONE*>(m_packet.data())->zoneId);
		return;
	}
	if (m_owner == login && type == ePacketType::S2C_Login)
	{
		if (m_packet.size() != sizeof(PACKET_S2C_LOGIN) || !client->IsLoginPending()) return;
		PACKET_S2C_LOGIN* result = reinterpret_cast<PACKET_S2C_LOGIN*>(m_packet.data());
		if (result->result)
		{
			client->SetAccountIndex(header->proxyHeader.accountIndex);
			PACKET_S2S_ENTER_WORLD enter{header->proxyHeader, {sizeof(PACKET_S2S_ENTER_WORLD) - sizeof(PROXY_HEADER), ePacketType::S2S_EnterWorld}};
			world->PostSend(MakeShared(enter));
			return;
		}
		client->SetLoginPending(false);
		if (!client->IsConnected())
		{
			m_clientConnManager->Release(client);
			return;
		}
	}
	if (m_owner != world && (type == ePacketType::S2C_Logout || (static_cast<uint16_t>(type) >= 3000 && static_cast<uint16_t>(type) < 4000)))
	{
		if (client->GetAccountIndex() != header->proxyHeader.accountIndex) return;
		world->PostSend(std::make_shared<std::vector<byte>>(m_packet));
		return;
	}
	if (m_owner == world && type == ePacketType::S2C_Login)
	{
		client->SetLoginPending(false);
		if (!client->IsConnected())
		{
			if (client->TryStartLogout())
			{
				PACKET_S2S_LEAVE_WORLD leave;
				leave.proxyHeader = header->proxyHeader;
				world->PostSend(MakeShared(leave));
			}
			return;
		}
	}
	if (type == ePacketType::S2C_Logout && m_owner == world)
	{
		if (client->GetAccountIndex() != header->proxyHeader.accountIndex) return;
		client->SetAccountIndex(0);
		client->SetZoneId(0);
		client->SetLogoutPending(false);
		if (client->IsConnected()) client->Disconnect();
		else m_clientConnManager->Release(client);
		return;
	}
	if (!client->IsConnected()) return;
	if (type != ePacketType::S2C_Login && type != ePacketType::S2C_CreateAccount && client->GetAccountIndex() != header->proxyHeader.accountIndex) return;
	if (m_owner != login && m_owner != world && m_owner != m_serverConnManager->GetZoneConnection(client->GetZoneId())) return;
	std::shared_ptr<std::vector<byte>> send = std::make_shared<std::vector<byte>>(m_packet.begin() + sizeof(PROXY_HEADER), m_packet.end());
	client->PostSend(send);
}

void CPacketTask::Cleanup()
{
	m_packet.clear();
	m_owner = nullptr;
	m_generation = NULL;
	m_type = eOwnerType::None;

	m_jobPool->Release(this);
}
