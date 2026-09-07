#include "PacketTask.h"
#include "ClientConnection.h"
#include "../Utility_Core/JobPool.h"
#include "../Common/PACKET.h"

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

void CPacketTask::ExecuteC2S()
{
	CClientConnection* owner = static_cast<CClientConnection*>(m_owner);
	PACKET_HEADER* packetHeader = reinterpret_cast<PACKET_HEADER*>(m_packet.data());
	ePacketType type = packetHeader->type;

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

	if (type == ePacketType::C2S_Logout)
	{
		if (!owner->TryStartLogout())
		{
			owner->Disconnect();
			return;
		}
	}

	auto relayPacket = std::make_shared<std::vector<byte>>(AttachProxyHeader(m_packet, { owner->GetSessionId(), owner->GetAccountIndex() }));

	eBackServerType dest = eBackServerType::None;
	if (1000 <= static_cast<uint16_t>(type) && static_cast<uint16_t>(type) <= 1999)
	{
		dest = eBackServerType::Login;
	}
	if (2000 <= static_cast<uint16_t>(type) && static_cast<uint16_t>(type) <= 2999)
	{
		dest = eBackServerType::Zone;
	}
	if (dest == eBackServerType::Login)
	{
		CTCPConnection* connection = m_serverConnManager->GetLoginConnection();

		if (!connection->PostSend(relayPacket) && type == ePacketType::C2S_Logout)
		{
			owner->SetLogoutPending(false);
			owner->Disconnect();
		}
	}
	if (dest == eBackServerType::Zone)
	{
		CTCPConnection* connection = m_serverConnManager->GetZoneConnection(owner->GetZoneId());
		if (connection == nullptr || !connection->PostSend(relayPacket)) owner->Disconnect();
	}
}

void CPacketTask::ExecuteS2C()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_HEADER* packetHeader = reinterpret_cast<PACKET_HEADER*>(m_packet.data() + sizeof(PROXY_HEADER));

	CClientConnection* clientConnection = m_clientConnManager->Find(proxyHeader->sessionId);
	if (clientConnection == nullptr) return;

	if (packetHeader->type == ePacketType::S2C_Logout)
	{
		clientConnection->SetAccountIndex(0);
		clientConnection->SetLogoutPending(false);

		if (clientConnection->IsConnected()) clientConnection->Disconnect();
		else m_clientConnManager->Release(clientConnection);

		return;
	}

	if (!clientConnection->IsConnected()) return;

	if (packetHeader->type == ePacketType::S2C_Login)
	{
		PACKET_S2C_LOGIN* packet = reinterpret_cast<PACKET_S2C_LOGIN*>(m_packet.data());
		if(packet->result == true) clientConnection->SetAccountIndex(proxyHeader->accountIndex);
	}
	if (packetHeader->type == ePacketType::S2C_SelectCharacter)
	{
		PACKET_S2C_SELECT_CHARACTER* packet = reinterpret_cast<PACKET_S2C_SELECT_CHARACTER*>(m_packet.data());
		if (packet->result == eZoneResult::Success) clientConnection->SetZoneId(packet->playerData.zoneId);
	}
	if (packetHeader->type == ePacketType::S2C_ChangeZone)
	{
		PACKET_S2C_CHANGE_ZONE* packet = reinterpret_cast<PACKET_S2C_CHANGE_ZONE*>(m_packet.data());
		if (packet->result == eZoneResult::Success) clientConnection->SetZoneId(packet->zoneId);
	}
	if (packetHeader->type == ePacketType::S2C_Respawn && packetHeader->size == sizeof(PACKET_S2C_RESPAWN) - sizeof(PROXY_HEADER))
	{
		PACKET_S2C_RESPAWN* packet = reinterpret_cast<PACKET_S2C_RESPAWN*>(m_packet.data());
		if (packet->result == eZoneResult::Success) clientConnection->SetZoneId(packet->zoneId);
	}

	auto sendPacket = std::make_shared<std::vector<byte>>(packetHeader->size);
	memcpy(sendPacket->data(), m_packet.data() + sizeof(PROXY_HEADER), packetHeader->size);
	clientConnection->PostSend(sendPacket);
}

void CPacketTask::Cleanup()
{
	m_packet.clear();
	m_owner = nullptr;
	m_generation = NULL;
	m_type = eOwnerType::None;

	m_jobPool->Release(this);
}
