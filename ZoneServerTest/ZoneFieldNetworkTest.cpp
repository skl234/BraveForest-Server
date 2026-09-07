#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include "../ZoneServer/ChannelManager.h"
#include "../ZoneServer/Session.h"
#include "../ZoneServer/CombatDamageCalculator.h"

#pragma comment(lib, "Ws2_32.lib")

enum class eTestCommandType : uint64_t
{
	Enter = 0,
	Move,
	Query,
	AttackMonster,
	Leave,
};

struct TEST_COMMAND
{
	eTestCommandType type;
	uint64_t characterIndex;
	uint64_t targetNum;
	VECTOR3 position;
};

struct TEST_RESPONSE
{
	bool result;
	uint64_t channelId;
	uint64_t objectNum;
	uint64_t eventCount;
	uint64_t visibleCount;
	eMonsterState monsterState;
	uint64_t monsterHP;
	float positionY;
	FIELD_VIEW_EVENT eventList[16];
};

class CTestChannelJob : public CJob
{
private:
	CChannel*		m_channel;
	CSession*		m_session;
	TEST_COMMAND	m_command;
	TEST_RESPONSE	m_response;
	HANDLE			m_hCompleteEvent;

public:
	CTestChannelJob(
		CChannel* _channel,
		CSession* _session,
		const TEST_COMMAND& _command) :
		m_channel(_channel),
		m_session(_session),
		m_command(_command),
		m_response({}),
		m_hCompleteEvent(NULL)
	{
		m_hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	~CTestChannelJob() override
	{
		if (m_hCompleteEvent != NULL) CloseHandle(m_hCompleteEvent);
	}

	void Execute() override
	{
		m_response.channelId = m_channel->GetChannelId();

		if (m_command.type == eTestCommandType::Enter)
		{
			CPlayer* player = m_channel->CreatePlayer(
				m_command.characterIndex,
				"TestPlayer",
				eClassState::Warrior,
				m_command.position);
			if (player != nullptr)
			{
				m_session->SetPlayer(
					m_command.characterIndex,
					m_channel->GetChannelId(),
					player);
				m_response.objectNum = player->GetNum();
				m_response.result = true;
			}
		}

		if (m_command.type == eTestCommandType::Move)
		{
			m_response.result = m_channel->MovePlayer(
				m_session->GetPlayer(),
				m_command.position);
		}

		if (m_command.type == eTestCommandType::Query)
		{
			CPlayer* player = m_session->GetPlayer();
			if (player != nullptr)
			{
				m_response.result = true;
				m_response.visibleCount =
					static_cast<uint64_t>(player->GetVisibleObjectList().size());
				m_response.positionY = player->GetPosition().y;

				std::vector<FIELD_VIEW_EVENT>& eventList = player->GetViewEventList();
				m_response.eventCount = static_cast<uint64_t>(eventList.size());
				if (m_response.eventCount > 16) m_response.eventCount = 16;
				for (uint64_t i = 0; i < m_response.eventCount; ++i)
				{
					m_response.eventList[i] = eventList[i];
				}
				player->ClearViewEventList();
			}
		}

		if (m_command.type == eTestCommandType::AttackMonster)
		{
			CMonster* monster = m_channel->GetMonsterManager()->Find(m_command.targetNum);
			if (monster != nullptr)
			{
				monster->TakeDamage(m_session->GetPlayer(), 100, GetTickCount64());
				m_response.result = true;
				m_response.monsterState = monster->GetState();
				m_response.monsterHP = monster->GetHP();
			}
		}

		if (m_command.type == eTestCommandType::Leave)
		{
			CPlayer* player = m_session->GetPlayer();
			if (player != nullptr)
			{
				m_channel->RemovePlayer(player);
				m_session->ClearPlayer();
				m_response.result = true;
			}
		}

		SetEvent(m_hCompleteEvent);
	}

	bool Wait(TEST_RESPONSE& _response)
	{
		DWORD result = WaitForSingleObject(m_hCompleteEvent, 5000);
		if (result != WAIT_OBJECT_0) return false;
		_response = m_response;
		return true;
	}
};

bool SendAll(SOCKET _socket, const char* _buffer, uint64_t _size)
{
	uint64_t sentSize = 0;
	while (sentSize < _size)
	{
		int result = send(
			_socket,
			_buffer + sentSize,
			static_cast<int>(_size - sentSize),
			0);
		if (result <= 0) return false;
		sentSize += static_cast<uint64_t>(result);
	}
	return true;
}

bool RecvAll(SOCKET _socket, char* _buffer, uint64_t _size)
{
	uint64_t recvSize = 0;
	while (recvSize < _size)
	{
		int result = recv(
			_socket,
			_buffer + recvSize,
			static_cast<int>(_size - recvSize),
			0);
		if (result <= 0) return false;
		recvSize += static_cast<uint64_t>(result);
	}
	return true;
}

void ClientWorker(
	SOCKET _socket,
	CChannelManager* _channelManager)
{
	CSession session;
	session.Activate(static_cast<uint64_t>(_socket), 1);

	while (true)
	{
		TEST_COMMAND command = {};
		if (!RecvAll(_socket, reinterpret_cast<char*>(&command), sizeof(command))) break;

		uint64_t channelId = 0;
		CChannel* channel = nullptr;
		if (command.type == eTestCommandType::Enter)
		{
			channel = _channelManager->SelectEnterChannel();
		}
		else
		{
			channelId = session.GetChannelId();
			channel = _channelManager->FindChannel(channelId);
		}

		TEST_RESPONSE response = {};
		if (channel != nullptr)
		{
			CTestChannelJob job(channel, &session, command);
			channel->PushJob(&job);
			job.Wait(response);
		}

		if (!SendAll(_socket, reinterpret_cast<const char*>(&response), sizeof(response))) break;
	}

	closesocket(_socket);
	session.Cleanup();
}

void RunTestServer(HANDLE _hReadyEvent)
{
	CChannelManager channelManager;
	if (!channelManager.Initialize(1, 3, "ZoneServer/FieldData/TestField.txt"))
	{
		SetEvent(_hReadyEvent);
		return;
	}
	if (!channelManager.Start())
	{
		SetEvent(_hReadyEvent);
		return;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN address = {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(39117);
	bind(listenSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address));
	listen(listenSocket, 3);
	SetEvent(_hReadyEvent);

	std::vector<std::thread> workerList;
	for (uint64_t i = 0; i < 3; ++i)
	{
		SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		workerList.emplace_back([clientSocket, &channelManager]()
			{
				ClientWorker(clientSocket, &channelManager);
			});
	}

	closesocket(listenSocket);
	uint64_t size = static_cast<uint64_t>(workerList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		workerList[i].join();
	}
	channelManager.Stop();
}

void RunLoadTestServer(HANDLE _hReadyEvent)
{
	CChannelManager channelManager;
	if (!channelManager.Initialize(2, 64, "ZoneServer/FieldData/TestField.txt"))
	{
		SetEvent(_hReadyEvent);
		return;
	}
	if (!channelManager.Start())
	{
		SetEvent(_hReadyEvent);
		return;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN address = {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(39118);
	bind(listenSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address));
	listen(listenSocket, SOMAXCONN);
	SetEvent(_hReadyEvent);

	std::vector<std::thread> workerList;
	workerList.reserve(100);
	for (uint64_t i = 0; i < 100; ++i)
	{
		SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		workerList.emplace_back([clientSocket, &channelManager]() { ClientWorker(clientSocket, &channelManager); });
	}

	closesocket(listenSocket);
	uint64_t size = static_cast<uint64_t>(workerList.size());
	for (uint64_t i = 0; i < size; ++i) workerList[i].join();
	channelManager.Stop();
}

SOCKET ConnectTestClient()
{
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN address = {};
	address.sin_family = AF_INET;
	address.sin_port = htons(39117);
	InetPton(AF_INET, L"127.0.0.1", &address.sin_addr);

	if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address)) == SOCKET_ERROR)
	{
		closesocket(clientSocket);
		return INVALID_SOCKET;
	}
	return clientSocket;
}

SOCKET ConnectLoadTestClient()
{
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN address = {};
	address.sin_family = AF_INET;
	address.sin_port = htons(39118);
	InetPton(AF_INET, L"127.0.0.1", &address.sin_addr);

	if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address)) == SOCKET_ERROR)
	{
		closesocket(clientSocket);
		return INVALID_SOCKET;
	}
	return clientSocket;
}

bool Request(SOCKET _socket, const TEST_COMMAND& _command, TEST_RESPONSE& _response)
{
	if (!SendAll(_socket, reinterpret_cast<const char*>(&_command), sizeof(_command))) return false;
	return RecvAll(_socket, reinterpret_cast<char*>(&_response), sizeof(_response));
}

bool HasEvent(
	const TEST_RESPONSE& _response,
	eFieldViewEventType _type,
	eFieldObjectType _objectType,
	uint64_t _objectNum)
{
	for (uint64_t i = 0; i < _response.eventCount; ++i)
	{
		const FIELD_VIEW_EVENT& event = _response.eventList[i];
		if (event.type != _type) continue;
		if (event.objectType != _objectType) continue;
		if (event.objectNum == _objectNum) return true;
	}
	return false;
}

bool Check(bool _condition, const char* _name)
{
	if (_condition)
	{
		std::cout << "[PASS] " << _name << std::endl;
		return true;
	}

	std::cout << "[FAIL] " << _name << std::endl;
	return false;
}

bool RunChannelSelectionTest()
{
	CChannelManager channelManager;
	if (!channelManager.Initialize(2, 4, "ZoneServer/FieldData/TestField.txt")) return false;
	if (!channelManager.Start()) return false;

	CChannel* firstChannel = channelManager.SelectEnterChannel();
	if (firstChannel == nullptr)
	{
		channelManager.Stop();
		return false;
	}

	TEST_COMMAND command = {};
	command.type = eTestCommandType::Enter;
	command.characterIndex = 9001;
	command.position = { 1.0f, 0.0f, 1.0f };
	TEST_RESPONSE response = {};
	CSession session;
	session.Activate(9001, 9001);
	CTestChannelJob enterJob(firstChannel, &session, command);
	firstChannel->PushJob(&enterJob);
	bool result = enterJob.Wait(response) && response.result;

	CChannel* secondChannel = channelManager.SelectEnterChannel();
	result = result && secondChannel != nullptr;
	if (secondChannel != nullptr)
	{
		result = result && firstChannel->GetChannelId() != secondChannel->GetChannelId();
	}

	command.type = eTestCommandType::Leave;
	CTestChannelJob leaveJob(firstChannel, &session, command);
	firstChannel->PushJob(&leaveJob);
	result = leaveJob.Wait(response) && response.result && result;

	channelManager.Stop();
	return result;
}

bool RunPlayerPoolTest()
{
	CChannelManager channelManager;
	if (!channelManager.Initialize(1, 1, "ZoneServer/FieldData/TestField.txt")) return false;
	if (!channelManager.Start()) return false;

	CChannel* channel = channelManager.FindChannel(1);
	if (channel == nullptr)
	{
		channelManager.Stop();
		return false;
	}

	CSession firstSession;
	firstSession.Activate(9101, 9101);
	TEST_COMMAND command = {};
	command.type = eTestCommandType::Enter;
	command.characterIndex = 9101;
	command.position = { 1.0f, 0.0f, 1.0f };
	TEST_RESPONSE response = {};
	CTestChannelJob firstEnterJob(channel, &firstSession, command);
	channel->PushJob(&firstEnterJob);
	bool result = firstEnterJob.Wait(response) && response.result && response.objectNum == 0;

	CSession secondSession;
	secondSession.Activate(9102, 9102);
	command.characterIndex = 9102;
	CTestChannelJob fullPoolJob(channel, &secondSession, command);
	channel->PushJob(&fullPoolJob);
	result = fullPoolJob.Wait(response) && !response.result && result;

	command.type = eTestCommandType::Leave;
	command.characterIndex = 9101;
	CTestChannelJob leaveJob(channel, &firstSession, command);
	channel->PushJob(&leaveJob);
	result = leaveJob.Wait(response) && response.result && result;

	command.type = eTestCommandType::Enter;
	command.characterIndex = 9102;
	CTestChannelJob reuseJob(channel, &secondSession, command);
	channel->PushJob(&reuseJob);
	result = reuseJob.Wait(response) && response.result && response.objectNum == 0 && result;

	command.type = eTestCommandType::Leave;
	CTestChannelJob finalLeaveJob(channel, &secondSession, command);
	channel->PushJob(&finalLeaveJob);
	result = finalLeaveJob.Wait(response) && response.result && result;

	channelManager.Stop();
	return result;
}

bool RunChannelReservationTest()
{
	CChannelManager channelManager;
	if (!channelManager.Initialize(2, 1, "ZoneServer/FieldData/TestField.txt")) return false;
	CChannel* firstChannel = channelManager.FindChannel(1);
	CChannel* secondChannel = channelManager.FindChannel(2);
	if (firstChannel == nullptr || secondChannel == nullptr) return false;
	if (!secondChannel->ReservePlayer()) return false;
	if (secondChannel->ReservePlayer()) return false;
	if (channelManager.SelectEnterChannel() != firstChannel) return false;
	CPlayer* player = secondChannel->CreateReservedPlayer(9150, "ReservedPlayer", eClassState::Warrior,
		{ 1.0f, 0.0f, 1.0f }, 1, 0);
	if (player == nullptr || secondChannel->GetReservedPlayerCount() != 0) return false;
	secondChannel->RemovePlayer(player);
	return true;
}

bool RunMonsterVectorTest()
{
	CChannelManager channelManager;
	if (!channelManager.Initialize(1, 1, "ZoneServer/FieldData/TestField.txt")) return false;

	CChannel* channel = channelManager.FindChannel(1);
	if (channel == nullptr) return false;

	CMonsterManager* monsterManager = channel->GetMonsterManager();
	if (monsterManager->GetSize() != 2) return false;

	CMonster* monster0 = monsterManager->Find(0);
	CMonster* monster1 = monsterManager->Find(1);
	if (monster0 == nullptr || monster1 == nullptr) return false;
	if (monster0->GetNum() != 0 || monster1->GetNum() != 1) return false;
	if (monster0->GetSpawnPosition().y != 0.0f) return false;
	if (monster0->GetWanderRadius() != 2.0f) return false;
	if (monster0->GetMaxAggroRange() != 8.0f) return false;
	if (monster0->GetRespawnRadius() != 1.5f) return false;
	if (monster0->GetRespawnDelayMs() != 5000) return false;
	if (monster0->IsInvincible()) return false;
	if (monster0->GetState() != eMonsterState::Idle) return false;
	return true;
}

bool RunMonsterBehaviorTest()
{
	MONSTER_SPAWN_INFO spawnInfo = {};
	spawnInfo.type = eMonsterType::Rat;
	spawnInfo.level = 2;
	spawnInfo.maxHP = 50;
	spawnInfo.experience = 25;
	spawnInfo.attackPower = 9;
	spawnInfo.aggressive = true;
	spawnInfo.detectionRange = 6.0f;
	spawnInfo.attackRange = 1.5f;
	spawnInfo.moveSpeed = 4.0f;
	spawnInfo.attackIntervalMs = 500;
	spawnInfo.spawnPosition = { 5.0f, 0.0f, 5.0f };
	spawnInfo.wanderRadius = 0.0f;
	spawnInfo.maxAggroRange = 8.0f;
	spawnInfo.respawnRadius = 1.0f;
	spawnInfo.respawnDelayMs = 10;
	std::vector<MONSTER_SPAWN_INFO> spawnInfoList;
	spawnInfoList.push_back(spawnInfo);

	CChannel channel;
	if (!channel.Initialize(1, 1, "ZoneServer/FieldData/TestField.txt", &spawnInfoList)) return false;
	CPlayer* player = channel.CreatePlayer(9201, "MonsterTarget", eClassState::Warrior, { 8.0f, 0.0f, 5.0f });
	CMonster* monster = channel.GetMonsterManager()->Find(0);
	if (player == nullptr || monster == nullptr) return false;

	uint64_t currentTime = GetTickCount64();
	monster->Update(currentTime, 500);
	if (monster->GetState() != eMonsterState::Chase) return false;
	monster->Update(currentTime + 500, 500);
	if (monster->GetPosition().x <= 5.0f) return false;
	uint64_t beforeHP = player->GetHP();
	monster->Update(currentTime + 1000, 500);
	if (monster->GetState() != eMonsterState::Attack) return false;
	if (beforeHP - player->GetHP() != spawnInfo.attackPower) return false;

	monster->TakeDamage(player, 1000, currentTime + 1100);
	if (monster->GetState() != eMonsterState::Dead || !monster->IsInvincible()) return false;
	monster->Update(currentTime + 1200, 500);
	if (monster->GetState() != eMonsterState::Idle) return false;
	if (monster->GetHP() != monster->GetMaxHP()) return false;
	if (monster->IsInvincible()) return false;

	channel.RemovePlayer(player);
	return true;
}

bool RunCombatBalanceTest()
{
	if (CCombatDamageCalculator::CalculatePlayerDamage(eClassState::Warrior, 1, eAttackType::BasicAttack) != 22) return false;
	if (CCombatDamageCalculator::CalculatePlayerDamage(eClassState::Archer, 20, eAttackType::BasicAttack) != 94) return false;
	if (CCombatDamageCalculator::CalculatePlayerDamage(eClassState::Archer, 20, eAttackType::RapidFire) != 28) return false;
	if (CCombatDamageCalculator::CalculateMonsterDamage(1) != 7) return false;
	if (CCombatDamageCalculator::CalculateMonsterDamage(6) != 17) return false;

	CPlayer player(0);
	player.Initialize(1, "ProgressionTest", eClassState::Warrior, 1, 0);
	player.TakeDamage(50);
	if (!player.AddExperience(100)) return false;
	if (player.GetLevel() != 2 || player.GetExperience() != 0) return false;
	if (player.GetMaxHP() != 120 || player.GetHP() != player.GetMaxHP()) return false;
	for (uint64_t i = 0; i < 50; ++i) player.AddExperience(60);
	if (player.GetLevel() != 5 || player.GetRequiredExperience() != 2500) return false;
	for (uint64_t i = 0; i < 142; ++i) player.AddExperience(180);
	if (player.GetLevel() != 10) return false;

	CPlayer maxLevelPlayer(1);
	maxLevelPlayer.Initialize(2, "MaxLevelTest", eClassState::Archer, 20, 0);
	if (maxLevelPlayer.AddExperience(1000)) return false;
	return maxLevelPlayer.GetExperience() == 0 && maxLevelPlayer.GetRequiredExperience() == 0;
}

bool RunHundredClientLoadTest(uint64_t& _elapsedTime)
{
	HANDLE hReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	std::thread serverThread([hReadyEvent]() { RunLoadTestServer(hReadyEvent); });
	WaitForSingleObject(hReadyEvent, 5000);

	bool result = true;
	std::vector<SOCKET> clientList(100, INVALID_SOCKET);
	std::vector<uint64_t> channelList(100, 0);
	uint64_t startTime = GetTickCount64();
	for (uint64_t i = 0; i < 100; ++i)
	{
		clientList[i] = ConnectLoadTestClient();
		if (clientList[i] == INVALID_SOCKET) result = false;
	}

	TEST_COMMAND command = {};
	TEST_RESPONSE response = {};
	uint64_t firstChannelCount = 0;
	uint64_t secondChannelCount = 0;
	for (uint64_t i = 0; i < 100 && result; ++i)
	{
		command.type = eTestCommandType::Enter;
		command.characterIndex = 10000 + i;
		command.position = { 1.0f, 0.0f, 1.0f };
		if (!Request(clientList[i], command, response) || !response.result) result = false;
		channelList[i] = response.channelId;
		if (response.channelId == 1) ++firstChannelCount;
		if (response.channelId == 2) ++secondChannelCount;
	}
	if (firstChannelCount != 50 || secondChannelCount != 50) result = false;

	for (uint64_t i = 0; i < 100 && result; ++i)
	{
		command.type = eTestCommandType::Query;
		command.characterIndex = 10000 + i;
		if (!Request(clientList[i], command, response) || !response.result) result = false;
		if (response.visibleCount != 50) result = false;
	}

	if (result)
	{
		command.type = eTestCommandType::Move;
		command.characterIndex = 10000;
		command.position = { 10.0f, 0.0f, 10.0f };
		if (!Request(clientList[0], command, response) || !response.result) result = false;

		command.type = eTestCommandType::Query;
		command.characterIndex = 10002;
		if (!Request(clientList[2], command, response)) result = false;
		if (!HasEvent(response, eFieldViewEventType::Leave, eFieldObjectType::Player, 0)) result = false;

		command.characterIndex = 10001;
		if (!Request(clientList[1], command, response)) result = false;
		if (response.eventCount != 0) result = false;
	}

	for (uint64_t i = 0; i < 100; ++i)
	{
		if (clientList[i] == INVALID_SOCKET) continue;
		command.type = eTestCommandType::Leave;
		command.characterIndex = 10000 + i;
		Request(clientList[i], command, response);
		closesocket(clientList[i]);
	}
	serverThread.join();
	CloseHandle(hReadyEvent);
	_elapsedTime = GetTickCount64() - startTime;
	return result && _elapsedTime < 30000;
}

int main()
{
	WSADATA wsaData = {};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	bool result = true;
	result = Check(
		RunChannelSelectionTest(),
		"ChannelManager selected the channel with fewer Players") && result;
	result = Check(
		RunPlayerPoolTest(),
		"PlayerManager rejected exhaustion and reused Pool index") && result;
	result = Check(
		RunChannelReservationTest(),
		"Channel transfer reservation prevented capacity races") && result;
	result = Check(
		RunMonsterVectorTest(),
		"MonsterManager used vector index and loaded Unity field values") && result;
	result = Check(
		RunMonsterBehaviorTest(),
		"Monster performed aggro, chase, attack, death and respawn states") && result;
	result = Check(
		RunCombatBalanceTest(),
		"Unity and ZoneServer combat damage plus level 1-10 progression matched") && result;

	HANDLE hReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	std::thread serverThread([hReadyEvent]() { RunTestServer(hReadyEvent); });
	WaitForSingleObject(hReadyEvent, 5000);

	SOCKET client1 = ConnectTestClient();
	SOCKET client2 = ConnectTestClient();
	SOCKET client3 = ConnectTestClient();
	result = Check(client1 != INVALID_SOCKET, "TestClient 1 connected") && result;
	result = Check(client2 != INVALID_SOCKET, "TestClient 2 connected") && result;
	result = Check(client3 != INVALID_SOCKET, "TestClient 3 connected") && result;

	TEST_COMMAND command = {};
	TEST_RESPONSE response = {};

	command.type = eTestCommandType::Enter;
	command.characterIndex = 1;
	command.position = { 1.0f, 7.5f, 1.0f };
	Request(client1, command, response);
	result = Check(response.result && response.channelId == 1, "Player 1 entered Channel 1") && result;

	command.type = eTestCommandType::Query;
	Request(client1, command, response);
	result = Check(
		response.visibleCount == 1 &&
		response.positionY == 0.0f &&
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Monster, 0),
		"Player 1 Y normalized and received nearby Monster index 0") && result;

	command.type = eTestCommandType::Enter;
	command.characterIndex = 2;
	command.position = { 2.0f, 0.0f, 1.0f };
	Request(client2, command, response);
	result = Check(response.result, "Player 2 entered") && result;

	command.type = eTestCommandType::Query;
	Request(client2, command, response);
	result = Check(
		response.visibleCount == 2 &&
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Player, 0),
		"Entering Player 2 received Player 1 information") && result;

	command.characterIndex = 1;
	Request(client1, command, response);
	result = Check(
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Player, 1),
		"Existing Player 1 received entering Player 2 information") && result;

	command.type = eTestCommandType::Enter;
	command.characterIndex = 3;
	command.position = { 10.0f, 0.0f, 10.0f };
	Request(client3, command, response);
	result = Check(response.result, "Player 3 entered distant Sector") && result;

	command.type = eTestCommandType::Query;
	Request(client3, command, response);
	result = Check(
		response.visibleCount == 2 &&
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Monster, 0) &&
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Monster, 1),
		"Player 3 received only objects in its 3x3 Sector range") && result;

	command.characterIndex = 1;
	Request(client1, command, response);
	result = Check(!HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Player, 2),
		"Player 1 did not receive distant Player 3") && result;

	command.type = eTestCommandType::Move;
	command.position = { 10.0f, 9.0f, 9.0f };
	Request(client1, command, response);
	result = Check(response.result, "Player 1 crossed Sector boundary") && result;

	command.type = eTestCommandType::Query;
	Request(client1, command, response);
	result = Check(
		response.eventCount == 3 &&
		HasEvent(response, eFieldViewEventType::Leave, eFieldObjectType::Player, 1) &&
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Player, 2) &&
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Monster, 1),
		"Sector move sent only visibility difference") && result;
	result = Check(
		!HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Monster, 0),
		"Known overlap Sector Monster was not sent twice") && result;

	command.type = eTestCommandType::Move;
	command.position = { 9.0f, 3.0f, 10.0f };
	Request(client1, command, response);
	command.type = eTestCommandType::Query;
	Request(client1, command, response);
	result = Check(response.eventCount == 0, "Move inside same Sector created no visibility event") && result;

	command.type = eTestCommandType::Move;
	command.position = { -1.0f, 0.0f, 10.0f };
	Request(client1, command, response);
	result = Check(!response.result, "Move outside Field was rejected") && result;

	command.type = eTestCommandType::AttackMonster;
	command.targetNum = 1;
	Request(client1, command, response);
	result = Check(
		response.result &&
		response.monsterState == eMonsterState::Dead &&
		response.monsterHP == 0,
		"Monster received damage and entered Dead state") && result;

	command.type = eTestCommandType::Leave;
	Request(client1, command, response);
	result = Check(response.result, "Player 1 left Field") && result;

	command.type = eTestCommandType::Query;
	command.characterIndex = 3;
	Request(client3, command, response);
	result = Check(
		HasEvent(response, eFieldViewEventType::Leave, eFieldObjectType::Player, 0),
		"Nearby Player 3 received Player 1 leave information") && result;

	command.type = eTestCommandType::Enter;
	command.characterIndex = 4;
	command.position = { 9.0f, 0.0f, 9.0f };
	Request(client1, command, response);
	result = Check(
		response.result && response.objectNum == 0,
		"Session acquired the released Player Pool index again") && result;

	command.type = eTestCommandType::Query;
	command.characterIndex = 3;
	Request(client3, command, response);
	result = Check(
		HasEvent(response, eFieldViewEventType::Enter, eFieldObjectType::Player, 0),
		"Nearby Player received reused Player object information") && result;

	command.type = eTestCommandType::Leave;
	command.characterIndex = 4;
	Request(client1, command, response);
	command.characterIndex = 2;
	Request(client2, command, response);
	command.characterIndex = 3;
	Request(client3, command, response);

	closesocket(client1);
	closesocket(client2);
	closesocket(client3);
	serverThread.join();
	CloseHandle(hReadyEvent);

	uint64_t loadElapsedTime = 0;
	bool loadResult = RunHundredClientLoadTest(loadElapsedTime);
	result = Check(loadResult, "100 clients exchanged visibility across two isolated Channels") && result;
	std::cout << "[INFO] 100-client load elapsed=" << loadElapsedTime << "ms" << std::endl;
	WSACleanup();

	if (!result) return 1;
	std::cout << "All Zone field network tests passed." << std::endl;
	return 0;
}
