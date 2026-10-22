#pragma once
#include <WinSock2.h>
#include <atomic>
#include <memory>
#include "../Network_Core/ServerService.h"
#include "../Network_Core/TCPAcceptor.h"
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/JobPool.h"
#include "../Utility_Core/MPSCExecutorManager.h"
#include "SessionManager.h"
#include "ChatManager.h"
#include "SessionController.h"
#include "PartyManager.h"
#include "CharacterDB.h"

class WorldService : public CServerService
{
private:
	CTCPAcceptor					m_acceptor;
	std::unique_ptr<CTCPConnection>	m_proxyConnection;
	std::atomic<bool>				m_proxyAccepted = false;

	CJobPool						m_packetTaskPool;

	CDBConnectionPool				m_gameDBPool;
	CCharacterDB					m_characterDB{ &m_gameDBPool };

	CZoneData						m_zoneData;

	CSessionManager					m_sessionManager;
	CPartyManager					m_partyManager;
	CChatManager					m_chatManager;
	CSessionController				m_sessionController;

	CMPSCExecutorManager			m_executorManager;

public:
	WorldService() = default;
	~WorldService() override = default;

	void	Stop();

protected:
	bool	OnInit() override;
	bool	OnRun() override;
	void	OnStop() override;
};
