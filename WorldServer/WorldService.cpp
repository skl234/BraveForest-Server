#include "WorldService.h"

#include <conio.h>
#include <iostream>
#include "PacketTask.h"
#include "WorldServerConfig.h"
#include "../ODBC_Core/ODBCDriver.h"
#include "../Common/PACKET.h"

constexpr uint64_t RECV_BUFFER_SIZE = 1024 * 400;
constexpr uint64_t MAX_SESSION_COUNT = 100000;
constexpr uint64_t PACKET_TASK_POOL_SIZE = 4096;

void WorldService::Stop()
{
	// IOCP 수신을 끝낸 뒤 로직을 종료. 정리 중에는 새 작업이 들어오지 않게 함.
	CServerService::Stop();
	m_executorManager.Stop();
	m_partyManager.Cleanup();
	m_sessionManager.Cleanup();
}


bool WorldService::OnInit()
{
	const WORLDSERVER_CONFIG* config = static_cast<const WORLDSERVER_CONFIG*>(m_config);
	if (config == nullptr || !m_zoneData.Load(config->zoneInfoPath)) return false;
	if (!CODBCDriver::GetInstance().Initialize()) return false;
	if (!m_gameDBPool.Initialize(config->dbUser, config->dbPassword, config->gameDsn, config->gameConnectionCount)) return false;

	m_proxyConnection = std::make_unique<CTCPConnection>(&m_server.GetOverlappedManager(), RECV_BUFFER_SIZE);
	m_sessionManager.Initialize(MAX_SESSION_COUNT);
	m_partyManager.Initialize(&m_sessionManager);
	m_chatManager.Initialize(&m_sessionManager, &m_partyManager);
	m_sessionController.Initialize(&m_sessionManager, m_proxyConnection.get(), &m_characterDB, &m_zoneData);
	m_sessionController.SetPartyManager(&m_partyManager);

	std::vector<std::unique_ptr<CJob>> taskList;
	taskList.reserve(PACKET_TASK_POOL_SIZE);
	for (uint64_t i = 0; i < PACKET_TASK_POOL_SIZE; ++i)
	{
		taskList.emplace_back(std::make_unique<CPacketTask>(&m_packetTaskPool, &m_sessionManager, &m_chatManager, &m_sessionController, &m_partyManager));
	}
	m_packetTaskPool.Initialize(std::move(taskList));
	if (!m_executorManager.Initialize(1)) return false;

	if (!m_acceptor.Initialize(m_config->ip, m_config->port)) return false;
	m_acceptor.SetCreateConnectionHandler([this](SOCKET _socket, SOCKADDR_IN _addr, CIOCP* _iocp)
		{
			// Proxy 연결은 한 번만 받음. 끊기면 World 재실행.
			if (m_proxyAccepted.exchange(true))
			{
				closesocket(_socket);
				return;
			}

			if (m_proxyConnection->Connect(_socket, _addr, _iocp))
			{
				std::cout << "Proxy Connected\n";
			}
		});
	m_acceptor.SetErrorHandler([](DWORD _error)
		{
			std::cout << "World Acceptor Error : " << _error << "\n";
		});

	m_proxyConnection->SetDisconnectHandler([this]()
		{
			std::cout << "Proxy Disconnected\n";
			CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
			if (task == nullptr) return;

			task->Initialize({}, m_proxyConnection.get());
			m_executorManager.Add(task, 0);
		});
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

				const uint64_t totalSize = sizeof(PROXY_HEADER) + header.packetHeader.size;
				if (recvBuffer.GetReadableSize() < totalSize) break;

				std::vector<byte> packet;
				if (!recvBuffer.Read(packet, totalSize))
				{
					m_proxyConnection->Disconnect();
					return;
				}

				CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
				if (task == nullptr)
				{
					m_proxyConnection->Disconnect();
					return;
				}

				task->Initialize(std::move(packet), m_proxyConnection.get());
				m_executorManager.Add(task, 0);
			}
		});
	m_proxyConnection->SetErrorHandler([](DWORD _error)
		{
			std::cout << "Proxy Connection Error : " << _error << "\n";
		});

	return true;
}

bool WorldService::OnRun()
{
	if (!m_executorManager.Start()) return false;

	if (!m_acceptor.Start(m_config->postAcceptSize, &m_server.GetIOCP(), &m_server.GetOverlappedManager()))
		return false;

	std::cout << "RunStart : " << m_config->ip << ":" << m_config->port << "\n";

	while (true)
	{
		if (_kbhit())
		{
			const char ch = _getch();
			if (ch == 'q' || ch == 'Q') break;
		}
		Sleep(1000);
	}

	return true;
}

void WorldService::OnStop()
{
	m_acceptor.Disconnect();
	if (m_proxyConnection != nullptr) m_proxyConnection->Disconnect();
}
