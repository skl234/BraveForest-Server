#include "WorldPacketCreator.h"
#include <cstddef>
#include "PartyManager.h"

std::vector<std::shared_ptr<std::vector<byte>>> CWorldPacketCreator::CreateClientList(const PROXY_HEADER& _sender, const std::vector<PROXY_HEADER>& _clientList, const std::vector<byte>& _packet)
{
	std::vector<std::shared_ptr<std::vector<byte>>> packetList;
	if (_clientList.empty() || _packet.size() < sizeof(PROXY_PACKET_HEADER)) return packetList;

	const PACKET_HEADER* header = reinterpret_cast<const PACKET_HEADER*>(_packet.data() + sizeof(PROXY_HEADER));
	const uint64_t dataSize = _packet.size() - sizeof(PROXY_HEADER);
	const uint64_t headerSize = offsetof(PACKET_S2S_SEND_CLIENT_LIST, clientList);
	if (header->size != dataSize) return packetList;
	if (dataSize > UINT16_MAX - (headerSize - sizeof(PROXY_HEADER)) - sizeof(PROXY_HEADER)) return packetList;

	// 최대 512명. 본문이 커지면 패킷 크기에 맞춰 더 나눔
	uint64_t maxCount = (UINT16_MAX - (headerSize - sizeof(PROXY_HEADER)) - dataSize) / sizeof(PROXY_HEADER);
	if (maxCount > 512) maxCount = 512;
	packetList.reserve((_clientList.size() - 1) / maxCount + 1);

	for (uint64_t i = 0; i < _clientList.size(); i += maxCount)
	{
		uint64_t clientCount = _clientList.size() - i;
		if (clientCount > maxCount) clientCount = maxCount;

		const uint64_t listSize = sizeof(PROXY_HEADER) * clientCount;
		const uint64_t size = headerSize + listSize + dataSize;
		std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
		PACKET_S2S_SEND_CLIENT_LIST* packet = reinterpret_cast<PACKET_S2S_SEND_CLIENT_LIST*>(buffer->data());

		packet->proxyHeader = _sender;
		packet->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2S_SendClientList };
		packet->clientCount = static_cast<uint16_t>(clientCount);

		memcpy(packet->clientList, _clientList.data() + i, listSize);
		memcpy(buffer->data() + headerSize + listSize, _packet.data() + sizeof(PROXY_HEADER), dataSize);
		packetList.push_back(std::move(buffer));
	}

	return packetList;
}


std::vector<byte> CWorldPacketCreator::CreateWorldChat(const std::string& _name, const char* _text, uint8_t _chatSize)
{
	if (_name.empty() || _name.size() > UINT8_MAX || _text == nullptr || _chatSize == 0) return {};

	const uint64_t size = offsetof(PACKET_S2C_WORLD_CHAT, text) + _name.size() + _chatSize;
	std::vector<byte> buffer(size);
	PACKET_S2C_WORLD_CHAT* packet = reinterpret_cast<PACKET_S2C_WORLD_CHAT*>(buffer.data());

	packet->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2C_WorldChat };
	packet->nameSize = static_cast<uint8_t>(_name.size());
	packet->chatSize = _chatSize;

	memcpy(packet->text, _name.data(), _name.size());
	memcpy(packet->text + _name.size(), _text, _chatSize);
	return buffer;
}

std::vector<byte> CWorldPacketCreator::CreatePartyChat(uint64_t _partyNum, const std::string& _name, const char* _text, uint8_t _chatSize)
{
	if (_partyNum == 0 || _name.empty() || _name.size() > 30 || _text == nullptr || _chatSize == 0) return {};
	const uint64_t size = offsetof(PACKET_S2C_PARTY_CHAT, text) + _name.size() + _chatSize;
	std::vector<byte> buffer(size);
	PACKET_S2C_PARTY_CHAT* packet = reinterpret_cast<PACKET_S2C_PARTY_CHAT*>(buffer.data());
	packet->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2C_PartyChat };
	packet->partyNum = _partyNum;
	packet->nameSize = static_cast<uint8_t>(_name.size());
	packet->chatSize = _chatSize;
	memcpy(packet->text, _name.data(), _name.size());
	memcpy(packet->text + _name.size(), _text, _chatSize);
	return buffer;
}


std::shared_ptr<std::vector<byte>> CWorldPacketCreator::CreateWhisper(const PROXY_HEADER& _target, const std::string& _name, const char* _text, uint8_t _chatSize)
{
	if (_name.empty() || _name.size() > 30 || _text == nullptr || _chatSize == 0) return nullptr;

	const uint64_t size = offsetof(PACKET_S2C_WHISPER, text) + _name.size() + _chatSize;
	std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
	PACKET_S2C_WHISPER* packet = reinterpret_cast<PACKET_S2C_WHISPER*>(buffer->data());

	packet->proxyHeader = _target;
	packet->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2C_Whisper };
	packet->nameSize = static_cast<uint8_t>(_name.size());
	packet->chatSize = _chatSize;

	memcpy(packet->text, _name.data(), _name.size());
	memcpy(packet->text + _name.size(), _text, _chatSize);
	return buffer;
}

std::shared_ptr<std::vector<byte>> CWorldPacketCreator::CreateWhisperResult(const PROXY_HEADER& _sender, const std::string& _name, const char* _text, uint8_t _chatSize, eWhisperResult _result)
{
	if (_name.empty() || _name.size() > 30 || (_text == nullptr && _chatSize != 0)) return nullptr;

	const uint64_t size = offsetof(PACKET_S2C_WHISPER_RESULT, text) + _name.size() + _chatSize;
	std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
	PACKET_S2C_WHISPER_RESULT* packet = reinterpret_cast<PACKET_S2C_WHISPER_RESULT*>(buffer->data());

	packet->proxyHeader = _sender;
	packet->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2C_WhisperResult };
	packet->result = _result;
	packet->nameSize = static_cast<uint8_t>(_name.size());
	packet->chatSize = _chatSize;

	memcpy(packet->text, _name.data(), _name.size());
	if (_chatSize > 0) memcpy(packet->text + _name.size(), _text, _chatSize);
	return buffer;
}


std::vector<std::shared_ptr<std::vector<byte>>> CWorldPacketCreator::CreatePartyEvents(const std::vector<PARTY_EVENT>& _events)
{
	std::vector<std::shared_ptr<std::vector<byte>>> result;
	for (uint64_t i = 0; i < _events.size(); ++i)
	{
		std::vector<std::shared_ptr<std::vector<byte>>> packets = CreatePartyEvent(_events[i]);
		for (uint64_t j = 0; j < packets.size(); ++j)
		{
			result.push_back(std::move(packets[j]));
		}
	}
	return result;
}

std::vector<std::shared_ptr<std::vector<byte>>> CWorldPacketCreator::CreatePartyEvent(const PARTY_EVENT& _event)
{
	if (_event.clientList.empty()) return {};
	if (_event.type == ePacketType::S2C_PartyApplicationList)
	{
		std::vector<std::shared_ptr<std::vector<byte>>> result;
		uint64_t start = 0;
		do
		{
			uint64_t count = _event.applicationList.size() - start;
			if (count > 128) count = 128;
			std::vector<byte> buffer(offsetof(PACKET_S2C_PARTY_APPLICATION_LIST, applicationList) + count * sizeof(PARTY_APPLICATION_INFO));
			PACKET_S2C_PARTY_APPLICATION_LIST* packet = reinterpret_cast<PACKET_S2C_PARTY_APPLICATION_LIST*>(buffer.data());
			packet->packetHeader = { static_cast<uint16_t>(buffer.size() - sizeof(PROXY_HEADER)), _event.type };
			packet->partyNum = _event.partyNum;
			packet->createNum = _event.createNum;
			packet->totalCount = _event.applicationList.size();
			packet->startIndex = start;
			packet->count = static_cast<uint16_t>(count);
			if (count != 0) memcpy(packet->applicationList, _event.applicationList.data() + start, count * sizeof(PARTY_APPLICATION_INFO));
			std::vector<std::shared_ptr<std::vector<byte>>> packets = CreateClientList(_event.clientList[0], _event.clientList, buffer);
			for (uint64_t i = 0; i < packets.size(); ++i) result.push_back(std::move(packets[i]));
			start += count;
		} while (start < _event.applicationList.size());
		return result;
	}
	if (_event.type == ePacketType::S2S_SetParty)
	{
		PACKET_S2S_SET_PARTY packet{};
		packet.proxyHeader = _event.clientList[0];
		packet.packetHeader = { sizeof(packet) - sizeof(PROXY_HEADER), _event.type };
		packet.partyNum = _event.partyNum;
		packet.characterIndex = _event.status.characterIndex;
		packet.zoneId = _event.status.zoneId;
		packet.channelId = _event.status.channelId;
		const uint64_t size = offsetof(PACKET_S2S_SEND_SERVER, packet) + sizeof(packet);
		std::shared_ptr<std::vector<byte>> buffer = std::make_shared<std::vector<byte>>(size);
		PACKET_S2S_SEND_SERVER* relay = reinterpret_cast<PACKET_S2S_SEND_SERVER*>(buffer->data());
		relay->proxyHeader = packet.proxyHeader;
		relay->packetHeader = { static_cast<uint16_t>(size - sizeof(PROXY_HEADER)), ePacketType::S2S_SendServer };
		relay->serverType = eBackServerType::Zone;
		relay->zoneId = packet.zoneId;
		memcpy(relay->packet, &packet, sizeof(packet));
		return { buffer };
	}
	std::vector<byte> buffer;
	if (_event.type == ePacketType::S2C_PartyInfo)
	{
		buffer.resize(offsetof(PACKET_S2C_PARTY_INFO, data) + _event.name.size() + _event.memberList.size() * sizeof(PARTY_MEMBER_INFO));
		PACKET_S2C_PARTY_INFO* packet = reinterpret_cast<PACKET_S2C_PARTY_INFO*>(buffer.data());
		packet->partyNum = _event.partyNum;
		packet->createNum = _event.createNum;
		packet->leaderCharacterIndex = _event.leaderCharacterIndex;
		packet->memberCount = static_cast<uint8_t>(_event.memberList.size());
		packet->nameSize = static_cast<uint8_t>(_event.name.size());
		memcpy(packet->data, _event.name.data(), _event.name.size());
		if (!_event.memberList.empty()) memcpy(packet->data + _event.name.size(), _event.memberList.data(), _event.memberList.size() * sizeof(PARTY_MEMBER_INFO));
	}
	else if (_event.type == ePacketType::S2C_PartyList)
	{
		uint64_t size = offsetof(PACKET_S2C_PARTY_LIST, data);
		for (uint64_t i = 0; i < _event.partyList.size(); ++i)
		{
			const PARTY_LIST_ENTRY& entry = _event.partyList[i];
			size += offsetof(PARTY_LIST_INFO, data) + entry.name.size() + entry.memberList.size() * sizeof(PARTY_MEMBER_INFO);
		}
		buffer.resize(size);
		PACKET_S2C_PARTY_LIST* packet = reinterpret_cast<PACKET_S2C_PARTY_LIST*>(buffer.data());
		packet->page = _event.page;
		packet->totalCount = _event.totalCount;
		packet->partyCount = static_cast<uint8_t>(_event.partyList.size());
		uint64_t offset = offsetof(PACKET_S2C_PARTY_LIST, data);
		for (uint64_t i = 0; i < _event.partyList.size(); ++i)
		{
			const PARTY_LIST_ENTRY& entry = _event.partyList[i];
			PARTY_LIST_INFO* info = reinterpret_cast<PARTY_LIST_INFO*>(buffer.data() + offset);
			info->partyNum = entry.partyNum;
			info->createNum = entry.createNum;
			info->leaderCharacterIndex = entry.leaderCharacterIndex;
			info->memberCount = static_cast<uint8_t>(entry.memberList.size());
			info->nameSize = static_cast<uint8_t>(entry.name.size());
			memcpy(info->data, entry.name.data(), entry.name.size());
			if (!entry.memberList.empty()) memcpy(info->data + entry.name.size(), entry.memberList.data(), entry.memberList.size() * sizeof(PARTY_MEMBER_INFO));
			offset += offsetof(PARTY_LIST_INFO, data) + entry.name.size() + entry.memberList.size() * sizeof(PARTY_MEMBER_INFO);
		}
	}
	else if (_event.type == ePacketType::S2C_PartyApplicationState)
	{
		buffer.resize(sizeof(PACKET_S2C_PARTY_APPLICATION_STATE));
		PACKET_S2C_PARTY_APPLICATION_STATE* packet = reinterpret_cast<PACKET_S2C_PARTY_APPLICATION_STATE*>(buffer.data());
		packet->requestNum = _event.requestNum;
		packet->partyNum = _event.partyNum;
		packet->createNum = _event.createNum;
		packet->active = _event.active;
		packet->result = _event.result;
	}
	else if (_event.type == ePacketType::S2C_PartyMemberStatus)
	{
		buffer.resize(sizeof(PACKET_S2C_PARTY_MEMBER_STATUS));
		PACKET_S2C_PARTY_MEMBER_STATUS* packet = reinterpret_cast<PACKET_S2C_PARTY_MEMBER_STATUS*>(buffer.data());
		packet->partyNum = _event.partyNum;
		packet->status = _event.status;
	}
	else if (_event.type == ePacketType::S2C_PartyInvite)
	{
		buffer.resize(offsetof(PACKET_S2C_PARTY_INVITE, name) + _event.name.size());
		PACKET_S2C_PARTY_INVITE* packet = reinterpret_cast<PACKET_S2C_PARTY_INVITE*>(buffer.data());
		packet->inviteNum = _event.inviteNum;
		packet->partyNum = _event.partyNum;
		packet->nameSize = static_cast<uint8_t>(_event.name.size());
		memcpy(packet->name, _event.name.data(), _event.name.size());
	}
	else if (_event.type == ePacketType::S2C_CancelPartyInvite)
	{
		buffer.resize(sizeof(PACKET_S2C_CANCEL_PARTY_INVITE));
		PACKET_S2C_CANCEL_PARTY_INVITE* packet = reinterpret_cast<PACKET_S2C_CANCEL_PARTY_INVITE*>(buffer.data());
		packet->inviteNum = _event.inviteNum;
		packet->reason = _event.result;
	}
	else return {};
	PROXY_PACKET_HEADER* header = reinterpret_cast<PROXY_PACKET_HEADER*>(buffer.data());
	header->packetHeader = { static_cast<uint16_t>(buffer.size() - sizeof(PROXY_HEADER)), _event.type };
	return CreateClientList(_event.clientList[0], _event.clientList, buffer);
}

std::shared_ptr<std::vector<byte>> CWorldPacketCreator::CreatePartyResult(const PROXY_HEADER& _target, ePacketType _request, ePartyResult _result, uint64_t _number)
{
	PACKET_S2C_PARTY_RESULT packet{};
	packet.proxyHeader = _target;
	packet.packetHeader = { sizeof(packet) - sizeof(PROXY_HEADER), ePacketType::S2C_PartyResult };
	packet.requestType = _request;
	packet.result = _result;
	packet.number = _number;
	return MakeShared(packet);
}
