#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <algorithm>
#include "CellMap.h"
typedef unsigned char byte;
#include "../../Common/PACKET.h"

enum class eDummyState { Waiting, Connecting, Login, CharacterList, Select, Enter, Online, Leave, Logout, Closed, Failed };

struct DUMMY_CONNECTION
{
	SOCKET					socket = INVALID_SOCKET;
	eDummyState			state = eDummyState::Waiting;
	uint64_t				index = 0;
	uint64_t				characterIndex = 0;
	uint64_t				zoneId = 0;
	uint64_t				channelId = 0;
	uint64_t				stateTime = 0;
	uint64_t				lastMoveTime = 0;
	uint64_t				nextMoveTime = 0;
	uint64_t				nextWanderTime = 0;
	ZONE_VECTOR3			anchor{};
	ZONE_VECTOR3			position{};
	float					rotationY = 0;
	uint64_t				pathIndex = 0;
	std::vector<ZONE_VECTOR3> path;
	std::vector<byte>		recvBuffer;
	std::vector<byte>		sendBuffer;
	size_t					sendOffset = 0;
	bool					entered = false;
	bool					left = false;
	bool					loggedOut = false;
};

class CDummyClient
{
private:
	std::vector<DUMMY_CONNECTION> m_connectionList;
	CCellMap				m_mapList[3];
	std::mt19937			m_random{ std::random_device{}() };
	std::ofstream			m_log;
	std::string				m_ip = "192.168.219.100";
	uint16_t				m_port = 30002;
	uint64_t				m_count = 500;
	uint64_t				m_startedCount = 0;
	uint64_t				m_nextConnectTime = 0;
	uint64_t				m_stopTime = 0;
	uint64_t				m_rxPackets = 0;
	uint64_t				m_discardPackets = 0;
	uint64_t				m_movePackets = 0;
	uint64_t				m_wanderCount = 0;
	uint64_t				m_skippedCount = 0;
	uint64_t				m_chatCount = 0;
	uint64_t				m_receivedChatCount = 0;
	uint64_t				m_logoutResetCount = 0;
	float					m_maxDistance = 0;
	bool					m_stopping = false;

public:
	bool Initialize();
	bool GenerateAccounts();
	bool VerifyPaths();
	int Run(uint64_t _durationSeconds = 0, bool _autoChat = false);

private:
	void StartConnect(DUMMY_CONNECTION& _connection, uint64_t _now);
	void Connected(DUMMY_CONNECTION& _connection, uint64_t _now);
	void Receive(DUMMY_CONNECTION& _connection, uint64_t _now);
	void FlushSend(DUMMY_CONNECTION& _connection);
	void ReadPacket(DUMMY_CONNECTION& _connection, const byte* _packet, uint16_t _size, uint64_t _now);
	void TickMove(DUMMY_CONNECTION& _connection, uint64_t _now);
	void SendMove(DUMMY_CONNECTION& _connection, bool _changed);
	void ChatRandom();
	void StopAll(uint64_t _now);
	void BeginLogout(DUMMY_CONNECTION& _connection, uint64_t _now);
	void Close(DUMMY_CONNECTION& _connection, const std::string& _error = "");
	void Send(DUMMY_CONNECTION& _connection, const void* _packet, size_t _size);
	void Log(const std::string& _text);
	void Report(bool _final);
};
