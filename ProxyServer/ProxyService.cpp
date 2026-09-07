#include "ProxyService.h"
#include <conio.h>
#include <iostream>
#include "Monitor.h"
#include "../Common/PACKET.h"
#include "../Utility_Core/LogManager.h"

bool CProxyService::OnInit()
{
	if (m_config == nullptr) return false;

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 1. Client Connection Pool
	// ------------------------------------------------------------------------------------------------------------------------------------
	m_clientConnectionManager.Initialize(m_config->maxClientConnection, &m_server.GetOverlappedManager());
	// ------------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 2. Backend Connection
	// ------------------------------------------------------------------------------------------------------------------------------------
	if (!m_serverConnectionManager.Initialize(&m_server.GetIOCP(), &m_server.GetOverlappedManager(), m_config->backServerInfoList))
	{
		return false;
	}
	CTCPConnection* loginConnection = m_serverConnectionManager.GetLoginConnection();
	if (loginConnection == nullptr) return false;
	if (m_serverConnectionManager.GetZoneConnectionList().size() <= 1) return false;
	// ------------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 3. PacketTask Pool
	// ------------------------------------------------------------------------------------------------------------------------------------
	std::vector<std::unique_ptr<CJob>> taskList;
	taskList.reserve(1280000);
	for (uint64_t i = 0; i < 1280000; ++i)
	{
		taskList.emplace_back(std::make_unique<CPacketTask>(&m_packetTaskPool, &m_clientConnectionManager, &m_serverConnectionManager));
	}
	m_packetTaskPool.Initialize(std::move(taskList));
	// ------------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 4. Logic Executor
	// ------------------------------------------------------------------------------------------------------------------------------------
	if (!m_executorManager.Initialize(m_config->logicThreadSize)) return false;
	if (!m_executorManager.Start()) return false;
	// ------------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 5. Acceptor Handler Init
	if (!m_acceptor.Initialize(m_config->ip, m_config->port)) return false;
	m_acceptor.SetCreateConnectionHandler([this](SOCKET _socket, SOCKADDR_IN _addr, CIOCP* _iocp)
		{
			CClientConnection* connection = m_clientConnectionManager.Acquire();
			if (connection == nullptr)
			{
				closesocket(_socket);
				return;
			}
			connection->Connect(_socket, _addr, _iocp);
		});
	m_acceptor.SetErrorHandler([](DWORD _error)
		{
		});
	// ------------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 6. CliencConnection Handler Init
	auto& connList = m_clientConnectionManager.GetList();
	uint64_t connSize = connList.size();
	for (uint64_t i = 0; i < connSize; ++i)
	{
		CClientConnection* connection = connList[i].get();
		connection->SetDisconnectHandler([connection, loginConnection, this]()
			{
				uint64_t accountIndex = connection->GetAccountIndex();
				if (accountIndex == 0)
				{
					m_clientConnectionManager.Release(connection);
					return;
				}

				if (!connection->TryStartLogout()) return;

				PACKET_C2S_LEAVE_ZONE leaveZonePacket;
				auto leavePacket = MakeShared(leaveZonePacket);
				auto zoneRelayPacket = std::make_shared<std::vector<byte>>(
					AttachProxyHeader(*leavePacket, { connection->GetSessionId(), accountIndex }));
				CTCPConnection* zoneConnection = m_serverConnectionManager.GetZoneConnection(connection->GetZoneId());
				if (zoneConnection != nullptr) zoneConnection->PostSend(zoneRelayPacket);

				PACKET_C2S_LOGOUT logoutPacket;
				auto packet = MakeShared(logoutPacket);
				auto relayPacket = std::make_shared<std::vector<byte>>(
					AttachProxyHeader(*packet, { connection->GetSessionId(), accountIndex }));

				if (!loginConnection->PostSend(relayPacket))
				{
					connection->SetLogoutPending(false);
					connection->SetAccountIndex(0);
					m_clientConnectionManager.Release(connection);
				}
			});
		connection->SetRecvHandler([connection, this]()
			{
				CRingBuffer& recvBuffer = connection->GetRecvBuffer();

				while (true)
				{
					PACKET_HEADER header{};
					if (!recvBuffer.Peek(header)) break;
					if (recvBuffer.GetReadableSize() < header.size) break;

					std::vector<byte> packet;
					if (!recvBuffer.Read(packet, header.size))
					{
						connection->Disconnect();
						break;
					}

					CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
					if (task == nullptr)
					{
						connection->Disconnect();
						return;
					}

					task->Initialize(std::move(packet), connection, connection->GetGeneration(), eOwnerType::Client);
					m_executorManager.Add(task, connection->GetSessionId());
				}
			});
		connection->SetErrorHandler([connection](DWORD _error)
			{
			});
	}
	// ------------------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------------------
	// 7. ServerConnection Handler Init
	loginConnection->SetDisconnectHandler([]()
		{
			//추후 로그인 서버 재연결 처리가 필요하다.
		});
	loginConnection->SetRecvHandler([this, loginConnection]()
		{
			CRingBuffer& recvBuffer = loginConnection->GetRecvBuffer();
			while (true)
			{
				PROXY_PACKET_HEADER header{};
				if (!recvBuffer.Peek(header)) break;

				uint64_t totalSize = sizeof(PROXY_HEADER) + header.packetHeader.size;
				if (recvBuffer.GetReadableSize() < totalSize) break;

				std::vector<byte> packet;
				if (!recvBuffer.Read(packet, totalSize))
				{
					loginConnection->Disconnect();
					break;
				}

				CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());

				if (task == nullptr)
				{
					loginConnection->Disconnect();
					return;
				}

				uint64_t sessionId = header.proxyHeader.sessionId;
				task->Initialize(std::move(packet), loginConnection, loginConnection->GetGeneration(), eOwnerType::Server);
				m_executorManager.Add(task, sessionId);
			}
		});

	loginConnection->SetErrorHandler([](DWORD error)
		{
			//로그 기록 후 재연결을 요청해야 한다.
		});

	std::vector<std::unique_ptr<CTCPConnection>>& zoneConnectionList = m_serverConnectionManager.GetZoneConnectionList();
	uint64_t zoneConnectionSize = static_cast<uint64_t>(zoneConnectionList.size());
	for (uint64_t i = 1; i < zoneConnectionSize; ++i)
	{
		if (zoneConnectionList[i] == nullptr) continue;
		CTCPConnection* zoneConnection = zoneConnectionList[i].get();
		zoneConnection->SetDisconnectHandler([]() {});
		zoneConnection->SetRecvHandler([this, zoneConnection]()
			{
				CRingBuffer& recvBuffer = zoneConnection->GetRecvBuffer();
				while (true)
				{
					PROXY_PACKET_HEADER header{};
					if (!recvBuffer.Peek(header)) break;
					uint64_t totalSize = sizeof(PROXY_HEADER) + header.packetHeader.size;
					if (recvBuffer.GetReadableSize() < totalSize) break;
					std::vector<byte> packet;
					if (!recvBuffer.Read(packet, totalSize)) { zoneConnection->Disconnect(); break; }
					CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
					if (task == nullptr) { zoneConnection->Disconnect(); return; }
					task->Initialize(std::move(packet), zoneConnection, zoneConnection->GetGeneration(), eOwnerType::Server);
					m_executorManager.Add(task, header.proxyHeader.sessionId);
				}
			});
		zoneConnection->SetErrorHandler([](DWORD _error) { (void)_error; });
	}

	// ------------------------------------------------------------------------------------------------------------------------------------

	CMonitor::GetInstance().Initialize(&m_clientConnectionManager, &m_serverConnectionManager, &m_server, &m_executorManager, &m_packetTaskPool);
	return true;
}

bool CProxyService::OnRun()
{
	if (!m_acceptor.Start(m_config->postAcceptSize, &m_server.GetIOCP(), &m_server.GetOverlappedManager()))
		return false;

	std::cout << "RunStart\n";

	while (true)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'q' || ch == 'Q') break;
		}
		Sleep(1000);
	}

	return true;
}

void CProxyService::OnStop()
{
	m_acceptor.Disconnect();
	auto& connectionList = m_clientConnectionManager.GetList();
	uint64_t listSize = connectionList.size();
	for (uint64_t i = 0; i < listSize; ++i)
	{
		connectionList[i]->Disconnect();
	}
	m_serverConnectionManager.DisconnectAll();
	m_executorManager.Stop();
}
