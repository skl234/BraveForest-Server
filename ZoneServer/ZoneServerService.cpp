#include "ZoneServerService.h"
#include <conio.h>
#include <iostream>
#include "PacketTask.h"
#include "../Common/PACKET.h"
#include "../ODBC_Core/ODBCDriver.h"

constexpr uint64_t ZONE_RECV_BUFFER_SIZE = 1024 * 400;

bool CZoneServerService::OnInit()
{
	ZONESERVER_CONFIG* config = static_cast<ZONESERVER_CONFIG*>(m_config);
	if (config == nullptr) return false;
	if (config->channelCount == 0 || config->channelCount > 40) return false;
	if (config->maxPlayerPerChannel == 0 || config->maxPlayerPerChannel > 256) return false;
	// 채널마다 동시에 DB 작업 하나까지 실행하므로 채널 수만큼은 확보
	if (config->dbConnectionCount < config->channelCount) return false;
	if (!CODBCDriver::GetInstance().Initialize()) return false;
	if (!m_dbConnectionPool.Initialize(config->dbUser, config->dbPassword, config->gameDsn, config->dbConnectionCount)) return false;

	// DB 클래스는 쿼리만 담당하고 연결은 서버의 풀 하나를 공유
	if (!m_zoneData.Load(config->zoneInfoPath)) return false;
	if (!m_characterDB.Initialize(&m_dbConnectionPool)) return false;
	if (!m_playerStatDB.Initialize(&m_dbConnectionPool)) return false;
	if (!m_monsterDBLoader.Initialize(&m_dbConnectionPool)) return false;

	if (!m_playerStatDB.Load()) return false;
	ZONE_INFO zoneInfo{};
	if (!m_zoneData.FindZone(config->zoneId, zoneInfo) || !zoneInfo.isActive) return false;
	ZONE_INFO townInfo{};
	if (!m_zoneData.FindZone(TOWN_ZONE_ID, townInfo) || !townInfo.isActive) return false;
	std::vector<MONSTER_SPAWN_INFO> typeList;
	std::vector<MONSTER_SPAWN_INFO> spawnInfoList;
	if (!m_monsterDBLoader.Load(typeList)) return false;
	if (!m_monsterSpawnData.Load(config->monsterSpawnPath, config->zoneId, typeList, spawnInfoList)) return false;
	if (spawnInfoList.size() > config->maxMonsterPerChannel) return false;
	if (!m_channelManager.Initialize(config->channelCount, config->maxPlayerPerChannel, config->fieldInfoPath, &spawnInfoList, &m_playerStatDB)) return false;
	for (uint64_t i = 0; i < config->channelCount; ++i)
	{
		CChannel* channel = m_channelManager.FindChannel(i + 1);
		channel->SetMonsterUpdateHandler([this](CChannel* _channel, CMonster* _monster)
			{ SendMonsterState(_channel, _monster); });
	}
	if (!m_channelManager.Start()) return false;
	m_sessionManager.Initialize(config->channelCount * config->maxPlayerPerChannel);

	m_proxyConnection = std::make_unique<CTCPConnection>(&m_server.GetOverlappedManager(), ZONE_RECV_BUFFER_SIZE);
	std::vector<std::unique_ptr<CJob>> taskList;
	taskList.reserve(config->packetTaskPoolSize);
	for (uint64_t i = 0; i < config->packetTaskPoolSize; ++i)
	{
		taskList.emplace_back(std::make_unique<CPacketTask>(&m_packetTaskPool, &m_sessionManager, &m_channelManager, &m_zoneData, &m_characterDB, config->zoneId));
	}
	m_packetTaskPool.Initialize(std::move(taskList));
	if (!m_acceptor.Initialize(config->ip, config->port)) return false;
	m_acceptor.SetCreateConnectionHandler([this](SOCKET _socket, SOCKADDR_IN _addr, CIOCP* _iocp)
		{
			if (m_proxyConnection->IsConnected())
			{
				closesocket(_socket);
				return;
			}
			m_proxyConnection->Connect(_socket, _addr, _iocp);
		});
	m_acceptor.SetErrorHandler([](DWORD _error)
		{ (void)_error; });
	m_proxyConnection->SetDisconnectHandler([]() {});
	m_proxyConnection->SetErrorHandler([](DWORD _error)
		{ (void)_error; });
	m_proxyConnection->SetRecvHandler([this]()
		{
			CRingBuffer& recvBuffer = m_proxyConnection->GetRecvBuffer();
			while (true)
			{
				PROXY_PACKET_HEADER header{};
				if (!recvBuffer.Peek(header)) break;
				if (header.packetHeader.size < sizeof(PACKET_HEADER))
				{
					m_proxyConnection->Disconnect();
					return;
				}
				uint64_t totalSize = sizeof(PROXY_HEADER) + header.packetHeader.size;
				if (recvBuffer.GetReadableSize() < totalSize) break;
				std::vector<byte> packet;
				if (!recvBuffer.Read(packet, totalSize))
				{
					m_proxyConnection->Disconnect();
					return;
				}
				CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
				if (task == nullptr) continue;
				CChannel* channel = nullptr;
				if (header.packetHeader.type == ePacketType::C2S_EnterZone)
					channel = m_channelManager.SelectEnterChannel();
				else
				{
					CSession* session = m_sessionManager.Find(header.proxyHeader.sessionId);
					if (session != nullptr) channel = m_channelManager.FindChannel(session->GetChannelId());
				}
				task->Initialize(std::move(packet), m_proxyConnection.get(), m_proxyConnection->GetGeneration(), channel);
				if (channel == nullptr)
				{
					task->Execute();
					continue;
				}
				channel->PushJob(task);
			}
		});
	return true;
}

bool CZoneServerService::OnRun()
{
	if (!m_acceptor.Start(m_config->postAcceptSize, &m_server.GetIOCP(), &m_server.GetOverlappedManager())) return false;
	std::cout << "ZoneServer RunStart\n";
	while (true)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'q' || ch == 'Q') break;
		}

		m_channelManager.ScheduleMonster(GetTickCount64());
		Sleep(10);
	}
	return true;
}

void CZoneServerService::OnStop()
{
	m_acceptor.Disconnect();
	if (m_proxyConnection != nullptr) m_proxyConnection->Disconnect();
	m_channelManager.Stop();
	ZONESERVER_CONFIG* config = static_cast<ZONESERVER_CONFIG*>(m_config);
	std::vector<CSession*> sessionList = m_sessionManager.GetActiveSessionList();
	uint64_t size = static_cast<uint64_t>(sessionList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		CSession* session = sessionList[i];
		CPlayer* player = session->GetPlayer();
		CChannel* channel = m_channelManager.FindChannel(session->GetChannelId());
		if (player != nullptr && channel != nullptr && config != nullptr)
		{
			PLAYER_DATA data{};
			data.accountIndex = session->GetAccountIndex();
			data.characterIndex = player->GetCharacterIndex();
			data.jobClass = static_cast<eJobClass>(player->GetClass());
			data.level = player->GetLevel();
			data.experience = player->GetExperience();
			data.zoneId = config->zoneId;
			data.position = {player->GetPosition().x, 0.0f, player->GetPosition().z};
			if (player->GetState() == ePlayerState::Die)
			{
				ZONE_INFO town{};
				if (m_zoneData.FindZone(TOWN_ZONE_ID, town) && town.isActive)
				{
					data.zoneId = town.zoneId;
					data.position = town.enterPosition;
				}
			}
			m_characterDB.Save(data);
			channel->RemovePlayer(player);
		}
		uint64_t sessionId = session->GetSessionId();
		m_sessionManager.Deactivate(sessionId);
	}
}

void CZoneServerService::SendMonsterState(CChannel* _channel, CMonster* _monster)
{
	CPacketTask::BroadcastMonsterState(_channel, &m_sessionManager, m_proxyConnection.get(), _monster);
}
