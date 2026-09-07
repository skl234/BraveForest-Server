#include "LoginService.h"
#include <conio.h>
#include <iostream>
#include "PacketTask.h"
#include "DBPoolManager.h"
#include "LoginServerConfig.h"
#include "../Common/PACKET.h"
#include "../ODBC_Core/ODBCDriver.h"
#include "../Utility_Core/LogManager.h"

constexpr uint64_t RECV_BUFFER_SIZE = 1024 * 400;

bool CLoginService::OnInit()
{
	LOGINSERVER_CONFIG* config = static_cast<LOGINSERVER_CONFIG*>(m_config);
	if (config == nullptr) return false;
	if (!m_zoneData.Load(config->zoneInfoPath)) return false;

	if (!CODBCDriver::GetInstance().Initialize()) return false;
	if (!CDBPoolManager<eDBType::Account>::GetInstance().Initialize(config->dbUser, config->dbPassword, config->accountDsn, config->accountConnectionCount))
	{
		return false;
	}
	if (!CDBPoolManager<eDBType::Game>::GetInstance().Initialize(config->dbUser, config->dbPassword, config->gameDsn, config->gameConnectionCount))
	{
		return false;
	}

	m_proxyConnection = std::make_unique<CTCPConnection>(&m_server.GetOverlappedManager(), RECV_BUFFER_SIZE);

	std::vector<std::unique_ptr<CJob>> taskList;
	taskList.reserve(config->packetTaskPoolSize);
	for (uint64_t i = 0; i < config->packetTaskPoolSize; ++i)
	{
		taskList.emplace_back(std::make_unique<CPacketTask>(&m_packetTaskPool, &m_zoneData));
	}
	m_packetTaskPool.Initialize(std::move(taskList));

	if (!m_executorManager.Initialize(1)) return false;
	if (!m_executorManager.Start()) return false;

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
		{
			CLogManager::GetInstance().Write(eLogLevel::eERROR, "Login Acceptor Error : %d", _error);
		});

	m_proxyConnection->SetDisconnectHandler([]()
		{
		});
	m_proxyConnection->SetRecvHandler([this]()
		{
			CRingBuffer& recvBuffer = m_proxyConnection->GetRecvBuffer();
			while (true)
			{
				PROXY_PACKET_HEADER header{};
				if (!recvBuffer.Peek(header)) break;

				uint64_t totalSize = sizeof(PROXY_HEADER) + header.packetHeader.size;
				if (recvBuffer.GetReadableSize() < totalSize) break;

				std::vector<byte> packet;
				if (!recvBuffer.Read(packet, totalSize))
				{
					m_proxyConnection->Disconnect();
					break;
				}

				CPacketTask* task = static_cast<CPacketTask*>(m_packetTaskPool.Acquire());
				if (task == nullptr)
				{
					m_proxyConnection->Disconnect();
					return;
				}

				task->Initialize(std::move(packet), m_proxyConnection.get(), m_proxyConnection->GetGeneration());
				m_executorManager.Add(task, 0);
			}
		});
	m_proxyConnection->SetErrorHandler([](DWORD _error)
		{
			CLogManager::GetInstance().Write(eLogLevel::eERROR, "Proxy Connection Error : %d", _error);
		});

	return true;
}

bool CLoginService::OnRun()
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

void CLoginService::OnStop()
{
	m_acceptor.Disconnect();
	if (m_proxyConnection != nullptr) m_proxyConnection->Disconnect();
	m_executorManager.Stop();
}
