#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <vector>
#include <string>
#include "../Utility_Core/Job.h"
#include "../Common/PACKET_HEADER.h"
#include "../Common/ZONE.h"

class CTCPConnection;
class CJobPool;
class CSessionManager;
class CChannelManager;
class CZoneData;
class CCharacterDB;
class CFieldObject;
class CPlayer;
class CChannel;
class CSession;
class CMonster;
struct PACKET_S2C_ATTACK_RESULT;

class CPacketTask : public CJob
{
private:
	std::vector<byte>	m_packet;
	CTCPConnection*		m_owner;
	uint64_t			m_generation;
	CJobPool*			m_jobPool;
	CSessionManager*	m_sessionManager;
	CChannelManager*	m_channelManager;
	CZoneData*			m_zoneData;
	CCharacterDB*		m_characterDB;
	uint64_t			m_zoneId;
	CChannel*			m_channel;
	bool				m_channelTransferEnter;
	PLAYER_DATA			m_transferData;
	std::string			m_transferName;

public:
	CPacketTask(CJobPool* _jobPool, CSessionManager* _sessionManager, CChannelManager* _channelManager, CZoneData* _zoneData, CCharacterDB* _characterDB, uint64_t _zoneId);
	~CPacketTask() override = default;
	void Initialize(std::vector<byte>&& _packet, CTCPConnection* _owner, uint64_t _generation, CChannel* _channel);
	void Execute() override;
	// 패킷 Job과 몬스터 Job이 같은 진입/이탈 전송을 사용
	static void FlushViewEventList(CChannel* _channel, CSessionManager* _sessionManager, CTCPConnection* _owner);
	static void BroadcastPlayerDead(CChannel* _channel, CSessionManager* _sessionManager, CTCPConnection* _owner, CPlayer* _player);
	static void BroadcastMonsterState(CChannel* _channel, CSessionManager* _sessionManager, CTCPConnection* _owner, CMonster* _monster);

private:
	void OnEnterZone();
	void OnLeaveZone();
	void OnMove();
	void OnAttackStart();
	void OnAttackExecute();
	void OnAttackCancel();
	void OnUsePotion();
	void OnChannelList();
	void OnChat();
	void OnChangeZone();
	void OnRespawn();
	bool OnChangeChannel();
	static void FillObjectInfo(CFieldObject* _object, FIELD_OBJECT_INFO& _info);
	void FlushViewEventList(CChannel* _channel);
	void BroadcastMove(CChannel* _channel, CPlayer* _player);
	void BroadcastAttackStart(CChannel* _channel, CPlayer* _player);
	void BroadcastAttackResult(CChannel* _channel, CPlayer* _player, CMonster* _monster, const PACKET_S2C_ATTACK_RESULT& _packet);
	void BroadcastLevelUp(CChannel* _channel, CPlayer* _player);
	static void SendObjectMove(CSession* _session, CFieldObject* _object, CTCPConnection* _owner);
	static void SendMonsterMove(CSession* _session, CMonster* _monster, CTCPConnection* _owner);
	void SendEnterFail(PROXY_HEADER* _proxyHeader, eZoneResult _result);
	void SendChangeChannel(CSession* _session, CChannel* _channel, CPlayer* _player, eZoneResult _result);
	bool IsValidPacket(PACKET_HEADER* _header);
	void Cleanup();
};
