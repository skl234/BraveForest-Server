#include "PacketTask.h"
#include <Windows.h>
#include <cstddef>
#include "AccountDB.h"
#include "../Common/PACKET.h"
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/JobPool.h"
#include "../Utility_Core/LogManager.h"

CPacketTask::CPacketTask(CJobPool* _jobPool):
	m_owner(nullptr),
	m_generation(NULL),
	m_jobPool(_jobPool)
{
}

void CPacketTask::Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation)
{
	m_packet = std::move(_packet);
	m_owner = _owner;
	m_generation = _generation;
}

void CPacketTask::Execute()
{
	if (m_owner == nullptr || m_owner->GetGeneration() != m_generation)
	{
		Cleanup();
		return;
	}

	if (m_packet.size() < sizeof(PROXY_PACKET_HEADER))
	{
		Cleanup();
		return;
	}

	PACKET_HEADER* packetHeader = reinterpret_cast<PACKET_HEADER*>(m_packet.data() + sizeof(PROXY_HEADER));
	if (packetHeader->size < sizeof(PACKET_HEADER) || packetHeader->size + sizeof(PROXY_HEADER) != m_packet.size())
	{
		Cleanup();
		return;
	}

	if (packetHeader->type == ePacketType::C2S_Login) OnLogin();
	if (packetHeader->type == ePacketType::C2S_Logout) OnLogout();
	if (packetHeader->type == ePacketType::C2S_CreateAccount) OnCreateAccount();

	Cleanup();
}

void CPacketTask::OnLogin()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_C2S_LOGIN* recvPacket = reinterpret_cast<PACKET_C2S_LOGIN*>(m_packet.data() + sizeof(PROXY_HEADER));

	uint64_t prefix = sizeof(PROXY_HEADER) + offsetof(PACKET_C2S_LOGIN, text);
	if (m_packet.size() < prefix) return;
	if (recvPacket->idSize < 5 || recvPacket->idSize > 15 || recvPacket->pwSize < 4 || recvPacket->pwSize > 15) return;
	uint64_t size = prefix + (static_cast<uint64_t>(recvPacket->idSize) + recvPacket->pwSize) * sizeof(wchar_t);
	if (m_packet.size() != size) return;

	std::wstring id(reinterpret_cast<wchar_t*>(recvPacket->text), recvPacket->idSize);
	std::wstring password(reinterpret_cast<wchar_t*>(recvPacket->text + recvPacket->idSize * sizeof(wchar_t)), recvPacket->pwSize);

	PACKET_S2C_LOGIN sendPacket;
	sendPacket.proxyHeader = *proxyHeader;
	sendPacket.result = false;

	uint64_t accountIndex = 0;
	bool result = CAccountDB::Login(id, password, accountIndex);

	if (result)
	{
		sendPacket.proxyHeader.accountIndex = accountIndex;
		sendPacket.result = true;
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[Login] Success, ID : %ls", id.c_str());
	}
	else
	{
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[Login] Fail, ID : %ls", id.c_str());
	}

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::OnLogout()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());

	PACKET_S2C_LOGOUT sendPacket;
	sendPacket.proxyHeader = *proxyHeader;

	sendPacket.result = CAccountDB::Logout(proxyHeader->accountIndex);

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::OnCreateAccount()
{
	PROXY_HEADER* proxyHeader = reinterpret_cast<PROXY_HEADER*>(m_packet.data());
	PACKET_S2C_CREATE_ACCOUNT sendPacket;
	sendPacket.proxyHeader = *proxyHeader;
	sendPacket.result = false;

	uint64_t packetPrefixSize = offsetof(PACKET_C2S_CREATE_ACCOUNT, text);
	if (m_packet.size() < sizeof(PROXY_HEADER) + packetPrefixSize)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	PACKET_C2S_CREATE_ACCOUNT* recvPacket = reinterpret_cast<PACKET_C2S_CREATE_ACCOUNT*>(
		m_packet.data() + sizeof(PROXY_HEADER));
	uint64_t requiredSize = sizeof(PROXY_HEADER) + packetPrefixSize +
		((static_cast<uint64_t>(recvPacket->idSize) + recvPacket->pwSize) * sizeof(wchar_t));

	if (recvPacket->idSize < 5 || recvPacket->idSize > 15 ||
		recvPacket->pwSize < 4 || recvPacket->pwSize > 15 ||
		m_packet.size() != requiredSize)
	{
		if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
		return;
	}

	std::wstring id(reinterpret_cast<wchar_t*>(recvPacket->text), recvPacket->idSize);
	std::wstring password(reinterpret_cast<wchar_t*>(recvPacket->text + recvPacket->idSize * sizeof(wchar_t)), recvPacket->pwSize);

	sendPacket.result = CAccountDB::Create(id, password);

	if (sendPacket.result)
	{
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[CreateAccount] Success, ID : %ls", id.c_str());
	}
	else
	{
		CLogManager::GetInstance().WriteW(eLogLevel::eINFO, L"[CreateAccount] Fail, ID : %ls", id.c_str());
	}

	if (m_owner->GetGeneration() == m_generation) m_owner->PostSend(MakeShared(sendPacket));
}

void CPacketTask::Cleanup()
{
	m_packet.clear();
	m_owner = nullptr;
	m_generation = NULL;

	m_jobPool->Release(this);
}
