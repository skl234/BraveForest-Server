#include "Monitor.h"
#include "ClientConnectionManager.h"
#include "../Common/PACKET_HEADER.h"
#include <iostream>

void CMonitor::Initialize(CClientConnectionManager* _clientConnManager, CServerConnectionManager* _servConnManager,
	CIOCPServer* _server, CMPSCExecutorManager* _mpscManager,
	CJobPool* _jobPool)
{
	m_clientConnManager = _clientConnManager;
	m_servConnManager = _servConnManager;
	m_server = _server;
	m_mpscManager = _mpscManager;
	m_jobPoolManager = _jobPool;
}

std::vector<byte> CMonitor::GetMonitorPacket()
{
	uint64_t currentClientConnection = m_clientConnManager->GetAcquireSize();
	std::vector<IOCP_TPS> iocpTpsList = m_server->GetTPSList();
	uint16_t iocpTpsCount = static_cast<uint16_t>(iocpTpsList.size());

	uint64_t jobCount = 0;
	std::vector<JOB_TPS> jobTpsList = m_mpscManager->GetTPSList();
	uint16_t jobTpsCount = static_cast<uint16_t>(jobTpsList.size());

	uint64_t jobPoolSize = m_jobPoolManager->GetPoolSize();

	uint16_t packetSize =
		sizeof(PACKET_HEADER) +
		sizeof(uint64_t) +
		sizeof(uint16_t) +
		static_cast<uint16_t>(sizeof(IOCP_TPS) * iocpTpsCount) +
		sizeof(uint64_t) +
		sizeof(uint16_t) +
		static_cast<uint16_t>(sizeof(JOB_TPS) * jobTpsCount) + 
		sizeof(uint64_t);

	PACKET_HEADER header{ packetSize, ePacketType::S2C_ProxyMonitor };

	std::vector<byte> packet(packetSize, 0);
	size_t offset = 0;

	memcpy(packet.data() + offset, &header, sizeof(header));
	offset += sizeof(header);

	memcpy(packet.data() + offset, &currentClientConnection, sizeof(currentClientConnection));
	offset += sizeof(currentClientConnection);

	memcpy(packet.data() + offset, &iocpTpsCount, sizeof(iocpTpsCount));
	offset += sizeof(iocpTpsCount);

	memcpy(packet.data() + offset, iocpTpsList.data(), sizeof(IOCP_TPS) * iocpTpsCount);
	offset += sizeof(IOCP_TPS) * iocpTpsCount;

	memcpy(packet.data() + offset, &jobCount, sizeof(jobCount));
	offset += sizeof(jobCount);

	memcpy(packet.data() + offset, &jobTpsCount, sizeof(jobTpsCount));
	offset += sizeof(jobTpsCount);

	memcpy(packet.data() + offset, jobTpsList.data(), sizeof(JOB_TPS) * jobTpsCount);
	offset += sizeof(JOB_TPS) * jobTpsCount;

	memcpy(packet.data() + offset, &jobPoolSize, sizeof(jobPoolSize));
	offset += sizeof(jobPoolSize);

	return packet;
}
