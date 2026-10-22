#pragma once
#include <WinSock2.h>
#include <vector>
#include <string>
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/Job.h"
#include "../Utility_Core/JobPool.h"

class CPacketTask : public CJob
{
private:
	std::vector<byte>	m_packet;
	CTCPConnection*		m_owner;
	uint64_t			m_generation;
	CJobPool*			m_jobPool;

public:
	CPacketTask(CJobPool* _jobPool);
	~CPacketTask() override = default;

	void Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation);
	void Execute() override;

private:
	void OnLogin();
	void OnLogout();
	void OnCreateAccount();
	void Cleanup();
};
