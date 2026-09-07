#pragma once
#include <WinSock2.h>
#include <vector>
#include <memory>
#include "ClientConnectionManager.h"
#include "ServerConnectionManager.h"
#include "../Utility_Core/MPSCExecutorManager.h"
#include "../Network_Core/TCPConnection.h"
#include "../Utility_Core/JobPool.h"

enum class eOwnerType : byte
{
	None = 0,
	Client,
	Server
};

class CJobPool;

class CPacketTask : public CJob
{
protected:
	std::vector<byte>			m_packet;		//작업할 Packet
	CTCPConnection*				m_owner;		//등록한 connection
	uint64_t					m_generation;	//등록한 시점의 generation
	eOwnerType					m_type;			//등록한 connection의 Type

	CJobPool*					m_jobPool;
	CClientConnectionManager*	m_clientConnManager;
	CServerConnectionManager*	m_serverConnManager;

public:
	CPacketTask(CJobPool* _pool, 
		CClientConnectionManager* _clientConnManager, 
		CServerConnectionManager* _serverConnManager);
	virtual ~CPacketTask() = default;

	void Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation, eOwnerType _type);
	void Execute() override;

private:
	void ExecuteC2S();
	void ExecuteS2C();
	void Cleanup();
};
