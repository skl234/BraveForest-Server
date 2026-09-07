#pragma once
#include <memory>
#include "ZoneServerConfig.h"
#include "../Common/ZoneData.h"
#include "CharacterDB.h"
#include "MonsterDBLoader.h"
#include "MonsterSpawnData.h"
#include "PlayerStatDB.h"
#include "ChannelManager.h"
#include "SessionManager.h"
#include "../Network_Core/ServerService.h"
#include "../Network_Core/TCPAcceptor.h"
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/JobPool.h"

class CZoneServerService : public CServerService
{
private:
	CTCPAcceptor					m_acceptor;
	std::unique_ptr<CTCPConnection>	m_proxyConnection;
	CDBConnectionPool				m_dbConnectionPool;
	CZoneData						m_zoneData;
	CCharacterDB					m_characterDB;
	CMonsterDBLoader				m_monsterDBLoader;
	CMonsterSpawnData				m_monsterSpawnData;
	CPlayerStatDB					m_playerStatDB;
	CChannelManager					m_channelManager;
	CSessionManager					m_sessionManager;
	CJobPool						m_packetTaskPool;

public:
	CZoneServerService() = default;
	~CZoneServerService() override = default;

protected:
	bool OnInit() override;
	bool OnRun() override;
	void OnStop() override;

private:
	void SendMonsterState(CChannel* _channel, CMonster* _monster);
};
