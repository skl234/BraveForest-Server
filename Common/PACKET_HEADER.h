#pragma once
#include <WinSock2.h>
#include <cstdint>

enum class ePacketType : uint16_t
{
	// ===== Proxy (0 ~ 999)
	C2S_ProxyEcho = 0,
	S2C_ProxyEcho,
	C2S_ProxyMonitor,
	S2C_ProxyMonitor,

	// ===== LoginServer (1000 ~ 1999)
	C2S_Login = 1000,
	S2C_Login,
	C2S_Logout,
	S2C_Logout,
	C2S_CreateAccount,
	S2C_CreateAccount,
	C2S_CharacterList,
	S2C_CharacterList,
	C2S_CreateCharacter,
	S2C_CreateCharacter,
	C2S_SelectCharacter,
	S2C_SelectCharacter,

	C2S_LoginEcho,
	S2C_LoginEcho,
	C2S_LoginMonitor,
	S2C_LoginMonitor,

	// ===== ZoneServer (2000 ~ 2999)
	C2S_EnterZone = 2000,
	S2C_EnterZone,
	C2S_LeaveZone,
	S2C_LeaveZone,
	C2S_Move,
	S2C_Move,
	C2S_AttackStart,
	S2C_AttackStart,
	C2S_AttackExecute,
	S2C_AttackResult,
	S2C_LevelUp,
	C2S_AttackCancel,
	C2S_UsePotion,
	S2C_UsePotion,
	C2S_ChannelList,
	S2C_ChannelList,
	C2S_ChangeChannel,
	S2C_ChangeChannel,
	C2S_ChangeZone,
	S2C_ChangeZone,
	S2C_FieldObjectEnter,
	S2C_FieldObjectLeave,
	S2C_MonsterState,
	// 2023: reserved
	S2C_MonsterMove = 2024,
	C2S_Chat,
	S2C_Chat,
	S2C_PlayerDead,
	C2S_Respawn,
	S2C_Respawn,
	S2C_PlayerHit,

	// ===== DummyPacket (10000~)
	C2S_DummyPacket8 = 10000,
	C2S_DummyPacket16,
	C2S_DummyPacket32,
	C2S_DummyPacket64,

	S2C_DummyPacket,

	End = 20000,
};

#pragma pack(1)

struct PROXY_HEADER
{
	uint64_t sessionId;
	uint64_t accountIndex;
};

struct PACKET_HEADER
{
	uint16_t	 size;
	ePacketType  type;
};

struct PROXY_PACKET_HEADER
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
};

#pragma pack()
