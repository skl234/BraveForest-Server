#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../Common/PACKET.h"

struct PARTY_EVENT;

class CWorldPacketCreator
{
public:
	static std::vector<std::shared_ptr<std::vector<byte>>>	CreateClientList(const PROXY_HEADER& _sender, const std::vector<PROXY_HEADER>& _clientList, const std::vector<byte>& _packet);

	static std::vector<byte>								CreateWorldChat(const std::string& _name, const char* _text, uint8_t _chatSize);
	static std::vector<byte>								CreatePartyChat(uint64_t _partyNum, const std::string& _name, const char* _text, uint8_t _chatSize);

	static std::shared_ptr<std::vector<byte>>				CreateWhisper(const PROXY_HEADER& _target, const std::string& _name, const char* _text, uint8_t _chatSize);
	static std::shared_ptr<std::vector<byte>>				CreateWhisperResult(const PROXY_HEADER& _sender, const std::string& _name, const char* _text, uint8_t _chatSize, eWhisperResult _result);

	static std::vector<std::shared_ptr<std::vector<byte>>>	CreatePartyEvents(const std::vector<PARTY_EVENT>& _events);
	static std::vector<std::shared_ptr<std::vector<byte>>>	CreatePartyEvent(const PARTY_EVENT& _event);
	static std::shared_ptr<std::vector<byte>>				CreatePartyResult(const PROXY_HEADER& _target, ePacketType _request, ePartyResult _result, uint64_t _number);
};
