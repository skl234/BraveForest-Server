#include "ServerConnectionManager.h"

constexpr uint64_t RECV_BUFFER_SIZE = 1024 * 400;

bool CServerConnectionManager::Initialize(CIOCP* _iocp, COverlappedManager* _overlManager, const std::vector<BACK_SERVER_INFO>& _infoList)
{
	uint64_t maxZoneId = 0;
	uint64_t infoSize = static_cast<uint64_t>(_infoList.size());
	for (uint64_t i = 0; i < infoSize; ++i)
	{
		if (_infoList[i].type != eBackServerType::Zone) continue;
		if (_infoList[i].serverId > maxZoneId) maxZoneId = _infoList[i].serverId;
	}

	// ZoneId와 Index를 그대로 일치시킨다.
	// 0번 Zone은 없으므로 0번 칸은 비워둔다.
	m_zoneServerConnectionList.resize(maxZoneId + 1);

	bool result = true;
	uint64_t size = static_cast<uint64_t>(_infoList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		std::unique_ptr<CTCPConnection> connection = std::make_unique<CTCPConnection>(_overlManager, RECV_BUFFER_SIZE);
		if (!connection->Connect(_infoList[i].ip, _infoList[i].port, _iocp))
		{
			result = false;
			break;
		}

		if (_infoList[i].type == eBackServerType::Login)
		{
			m_loginServerConnection = std::move(connection);
		}
		if (_infoList[i].type == eBackServerType::Zone)
		{
			uint64_t zoneId = _infoList[i].serverId;
			if (zoneId == 0 || zoneId >= m_zoneServerConnectionList.size() || m_zoneServerConnectionList[zoneId] != nullptr)
			{
				result = false;
				break;
			}
			m_zoneServerConnectionList[zoneId] = std::move(connection);
		}
	}

	if (!result)
	{
		m_loginServerConnection.reset();
		m_zoneServerConnectionList.clear();
	}

	return result;
}

void CServerConnectionManager::DisconnectAll()
{
	if (m_loginServerConnection != nullptr) m_loginServerConnection->Disconnect();
	uint64_t size = static_cast<uint64_t>(m_zoneServerConnectionList.size());
	for (uint64_t i = 1; i < size; ++i)
	{
		if (m_zoneServerConnectionList[i] != nullptr) m_zoneServerConnectionList[i]->Disconnect();
	}
}

CTCPConnection* CServerConnectionManager::GetLoginConnection()
{
	return m_loginServerConnection.get();
}

CTCPConnection* CServerConnectionManager::GetZoneConnection(uint64_t _zoneId)
{
	if (_zoneId == 0 || _zoneId >= m_zoneServerConnectionList.size()) return nullptr;
	return m_zoneServerConnectionList[_zoneId].get();
}

std::vector<std::unique_ptr<CTCPConnection>>& CServerConnectionManager::GetZoneConnectionList()
{
	return m_zoneServerConnectionList;
}
