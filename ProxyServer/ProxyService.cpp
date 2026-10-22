#include "ProxyService.h"
#include <conio.h>
#include <iostream>
#include "Monitor.h"
#include "../Common/PACKET.h"
#include <cstddef>
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
	CTCPConnection* worldConnection = m_serverConnectionManager.GetWorldConnection();
	if (worldConnection == nullptr) return false;
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
		connection->SetDisconnectHandler([connection, this]()
			{
				CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
				if (task == nullptr) return;
				task->Initialize({}, connection, connection->GetGeneration(), eOwnerType::Client);
				m_executorManager.Add(task, connection->GetSessionId());
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
	InitializeServerHandler(loginConnection);
	InitializeServerHandler(worldConnection);
	std::vector<std::unique_ptr<CTCPConnection>>& zoneList = m_serverConnectionManager.GetZoneConnectionList();
	for (uint64_t i = 1; i < zoneList.size(); ++i)
	{
		if (zoneList[i] != nullptr) InitializeServerHandler(zoneList[i].get());
	}

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
	uint64_t until = GetTickCount64() + 30000;
	while (m_clientConnectionManager.GetAcquireSize() != 0 && GetTickCount64() < until) Sleep(10);
	if (m_clientConnectionManager.GetAcquireSize() != 0) std::cerr << "Proxy: disconnect cleanup timed out\n";
	m_serverConnectionManager.DisconnectAll();
	m_executorManager.Stop();
}

void CProxyService::InitializeServerHandler(CTCPConnection* _connection)
{
	_connection->SetDisconnectHandler([]() {});
	_connection->SetErrorHandler([](DWORD _error) { (void)_error; });
	_connection->SetRecvHandler([this, _connection]()
		{
			CRingBuffer& recvBuffer = _connection->GetRecvBuffer();
			while (true)
			{
				PROXY_PACKET_HEADER header{};
				if (!recvBuffer.Peek(header)) break;
				if (header.packetHeader.size < sizeof(PACKET_HEADER))
				{
					_connection->Disconnect();
					return;
				}
				uint64_t totalSize = sizeof(PROXY_HEADER) + header.packetHeader.size;
				if (recvBuffer.GetReadableSize() < totalSize) break;
				std::vector<byte> packet;
				if (!recvBuffer.Read(packet, totalSize))
				{
					_connection->Disconnect();
					return;
				}
				if (_connection == m_serverConnectionManager.GetWorldConnection() && header.packetHeader.type == ePacketType::S2S_SendClientList)
				{
					uint64_t prefix = offsetof(PACKET_S2S_SEND_CLIENT_LIST, clientList);
					if (packet.size() < prefix) continue;
					PACKET_S2S_SEND_CLIENT_LIST* list = reinterpret_cast<PACKET_S2S_SEND_CLIENT_LIST*>(packet.data());
					uint64_t dataOffset = prefix + sizeof(PROXY_HEADER) * list->clientCount;
					if (packet.size() < dataOffset + sizeof(PACKET_HEADER)) continue;
					PACKET_HEADER* payload = reinterpret_cast<PACKET_HEADER*>(packet.data() + dataOffset);
					if (payload->size < sizeof(PACKET_HEADER) || dataOffset + payload->size != packet.size()) continue;
					// 같은 수신자 큐에 순서대로 넣고 나서 다음 World 패킷을 읽음.
					for (uint64_t i = 0; i < list->clientCount; ++i)
					{
						std::vector<byte> message(sizeof(PROXY_HEADER) + payload->size);
						memcpy(message.data(), &list->clientList[i], sizeof(PROXY_HEADER));
						memcpy(message.data() + sizeof(PROXY_HEADER), payload, payload->size);
						QueueServerPacket(_connection, std::move(message));
					}
				}
				else QueueServerPacket(_connection, std::move(packet));
			}
		});
}

void CProxyService::QueueServerPacket(CTCPConnection* _connection, std::vector<byte>&& _packet)
{
	PROXY_HEADER* header = reinterpret_cast<PROXY_HEADER*>(_packet.data());
	uint64_t sessionId = header->sessionId;
	CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
	if (task == nullptr)
	{
		_connection->Disconnect();
		return;
	}
	task->Initialize(std::move(_packet), _connection, _connection->GetGeneration(), eOwnerType::Server);
	m_executorManager.Add(task, sessionId);
}
