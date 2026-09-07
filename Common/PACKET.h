#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <vector>
#include <memory>
#include "PACKET_HEADER.h"
#include "CHARACTER.h"
#include "ZONE.h"

static std::vector<byte> AttachProxyHeader(const std::vector<byte>& _packet, const PROXY_HEADER& _header)
{
	std::vector<byte> newPacket; newPacket.resize(_packet.size() + sizeof(PROXY_HEADER));
	std::copy(reinterpret_cast<const byte*>(&_header), reinterpret_cast<const byte*>(&_header) + sizeof(PROXY_HEADER), newPacket.begin());
	std::copy(_packet.data(), _packet.data() + _packet.size(), newPacket.begin() + sizeof(PROXY_HEADER));

	return newPacket;
}

template<typename T>
std::shared_ptr<std::vector<byte>> MakeShared(const T& _packet)
{
	std::shared_ptr<std::vector<byte>> buffer =
		std::make_shared<std::vector<byte>>(sizeof(T));

	memcpy(buffer->data(), &_packet, sizeof(T));

	return buffer;
}

#pragma pack(1)

// ProxyServer ================================
struct PACKET_C2S_PROXY_ECHO
{
	PACKET_HEADER header;

	PACKET_C2S_PROXY_ECHO():
		header{ sizeof(PACKET_C2S_PROXY_ECHO), ePacketType::C2S_ProxyEcho } {}
};

struct PACKET_S2C_PROXY_ECHO
{
	PACKET_HEADER header;

	PACKET_S2C_PROXY_ECHO() :
		header{ sizeof(PACKET_S2C_PROXY_ECHO), ePacketType::S2C_ProxyEcho } {}
};

struct PACKET_C2S_PROXY_MONITOR
{
	PACKET_HEADER header;

	PACKET_C2S_PROXY_MONITOR() :
		header{ sizeof(PACKET_C2S_PROXY_MONITOR), ePacketType::C2S_ProxyMonitor } {}
};

struct PACKET_S2C_PROXY_MONITOR
{
	PACKET_HEADER header;
	uint64_t	  clientConnectionCount;
	uint64_t	  packetJobSize;

	PACKET_S2C_PROXY_MONITOR() :
		header{ sizeof(PACKET_S2C_PROXY_MONITOR), ePacketType::S2C_ProxyMonitor } {}
};

// LoginServer ================================
struct PACKET_C2S_LOGIN
{
	PACKET_HEADER header;
	int			  idSize;
	int			  pwSize;
	byte		  text[1];
};

struct PACKET_S2C_LOGIN
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
	bool		  result;

	PACKET_S2C_LOGIN() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_LOGIN) - sizeof(PROXY_HEADER), ePacketType::S2C_Login } {}
};

struct PACKET_C2S_LOGOUT
{
	PACKET_HEADER header;

	PACKET_C2S_LOGOUT() :
		header{ sizeof(PACKET_C2S_LOGOUT), ePacketType::C2S_Logout } {}
};

struct PACKET_S2C_LOGOUT
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
	bool		  result;

	PACKET_S2C_LOGOUT() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_LOGOUT) - sizeof(PROXY_HEADER), ePacketType::S2C_Logout },
		result(false) {}
};

struct PACKET_C2S_CREATE_ACCOUNT
{
	PACKET_HEADER header;
	int			  idSize;
	int			  pwSize;
	byte		  text[1];
};

struct PACKET_S2C_CREATE_ACCOUNT
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
	bool		  result;

	PACKET_S2C_CREATE_ACCOUNT() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_CREATE_ACCOUNT) - sizeof(PROXY_HEADER), ePacketType::S2C_CreateAccount } {}
};

struct PACKET_C2S_CHARACTERLIST
{
	PACKET_HEADER header;

	PACKET_C2S_CHARACTERLIST() :
		header{ sizeof(PACKET_C2S_CHARACTERLIST), ePacketType::C2S_CharacterList } {}
};

struct PACKET_S2C_CHARACTERLIST
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
	byte		  characterCount;
	//byte[]	  buff;
	//캐릭터 정보는 패킷 뒤쪽 버퍼에 이어서 저장한다.

	PACKET_S2C_CHARACTERLIST() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_CHARACTERLIST) - sizeof(PROXY_HEADER), ePacketType::S2C_CharacterList },
		characterCount(0) {}
};

struct PACKET_C2S_CREATE_CHARACTER
{
	PACKET_HEADER header;
	eJobClass	  jobClass;
	byte		  nameLen;
	//byte[]	  name;
	//닉네임은 패킷 뒤쪽 버퍼에 이어서 저장한다.

	PACKET_C2S_CREATE_CHARACTER() :
		header{ sizeof(PACKET_C2S_CREATE_CHARACTER), ePacketType::C2S_CreateCharacter },
		jobClass(eJobClass::Warrior),
		nameLen(0) {}
};

struct PACKET_S2C_CREATE_CHARACTER
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
	bool		  result;
	//CHARACTER	  character;
	//성공한 경우에만 생성된 캐릭터 정보를 패킷 뒤쪽 버퍼에 이어서 저장한다.

	PACKET_S2C_CREATE_CHARACTER() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_CREATE_CHARACTER) - sizeof(PROXY_HEADER), ePacketType::S2C_CreateCharacter },
		result(false) {}
};

struct PACKET_C2S_SELECT_CHARACTER
{
	PACKET_HEADER header;
	uint64_t characterIndex;

	PACKET_C2S_SELECT_CHARACTER() :
		header{ sizeof(PACKET_C2S_SELECT_CHARACTER), ePacketType::C2S_SelectCharacter },
		characterIndex(0) {}
};

struct PACKET_S2C_SELECT_CHARACTER
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	PLAYER_DATA playerData;

	PACKET_S2C_SELECT_CHARACTER() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_SELECT_CHARACTER) - sizeof(PROXY_HEADER), ePacketType::S2C_SelectCharacter },
		result(eZoneResult::Fail),
		playerData{} {}
};

struct PACKET_C2S_LOGIN_ECHO
{
	PACKET_HEADER header;

	PACKET_C2S_LOGIN_ECHO() :
		header{ sizeof(PACKET_C2S_LOGIN_ECHO), ePacketType::C2S_LoginEcho } {}
};

struct PACKET_S2C_LOGIN_ECHO
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;

	PACKET_S2C_LOGIN_ECHO() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_LOGIN_ECHO) - sizeof(PROXY_HEADER), ePacketType::S2C_LoginEcho } {}
};

struct PACKET_C2S_LOGIN_MONITOR
{
	PACKET_HEADER header;

	PACKET_C2S_LOGIN_MONITOR() :
		header{ sizeof(PACKET_C2S_LOGIN_MONITOR), ePacketType::C2S_LoginMonitor } {}
};

struct PACKET_S2C_LOGIN_MONITOR
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t	  packetJobSize;
	uint64_t      dbJobSize;

	PACKET_S2C_LOGIN_MONITOR() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_LOGIN_MONITOR) - sizeof(PROXY_HEADER), ePacketType::S2C_LoginMonitor } {}
};

// ZoneServer =================================
struct PACKET_C2S_ENTER_ZONE
{
	PACKET_HEADER header;
	uint64_t characterIndex;

	PACKET_C2S_ENTER_ZONE() :
		header{ sizeof(PACKET_C2S_ENTER_ZONE), ePacketType::C2S_EnterZone },
		characterIndex(0) {}
};

struct PACKET_S2C_ENTER_ZONE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	uint64_t channelId;
	uint64_t playerObjectNum;
	uint64_t experience;
	uint64_t requiredExperience;
	uint64_t objectCount;
	FIELD_OBJECT_INFO objectList[1];
};

struct PACKET_C2S_LEAVE_ZONE
{
	PACKET_HEADER header;
	PACKET_C2S_LEAVE_ZONE() : header{ sizeof(PACKET_C2S_LEAVE_ZONE), ePacketType::C2S_LeaveZone } {}
};

struct PACKET_S2C_LEAVE_ZONE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	PACKET_S2C_LEAVE_ZONE() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_LEAVE_ZONE) - sizeof(PROXY_HEADER), ePacketType::S2C_LeaveZone }, result(eZoneResult::Fail) {}
};

struct PACKET_C2S_MOVE
{
	PACKET_HEADER header;
	ZONE_VECTOR3 position;
	ZONE_VECTOR3 destination;
	float rotationY;
	bool destinationChanged;
	PACKET_C2S_MOVE() : header{ sizeof(PACKET_C2S_MOVE), ePacketType::C2S_Move }, position{}, destination{}, rotationY(0.0f), destinationChanged(false) {}
};

struct PACKET_S2C_MOVE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t playerObjectNum;
	ZONE_VECTOR3 position;
	ZONE_VECTOR3 destination;
	float rotationY;
	PACKET_S2C_MOVE() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_MOVE) - sizeof(PROXY_HEADER), ePacketType::S2C_Move }, playerObjectNum(0), position{}, destination{}, rotationY(0.0f) {}
};

struct PACKET_C2S_ATTACK_START
{
	PACKET_HEADER header;
	eAttackType attackType;
	eZoneObjectType targetType;
	uint64_t targetObjectNum;
	ZONE_VECTOR3 position;
	float rotationY;
	PACKET_C2S_ATTACK_START() : header{ sizeof(PACKET_C2S_ATTACK_START), ePacketType::C2S_AttackStart }, attackType(eAttackType::None), targetType(eZoneObjectType::Monster), targetObjectNum(0), position{}, rotationY(0.0f) {}
};

struct PACKET_S2C_ATTACK_START
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t playerObjectNum;
	eAttackType attackType;
	uint64_t targetObjectNum;
	ZONE_VECTOR3 position;
	float rotationY;
	PACKET_S2C_ATTACK_START() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_ATTACK_START) - sizeof(PROXY_HEADER), ePacketType::S2C_AttackStart }, playerObjectNum(0), attackType(eAttackType::None), targetObjectNum(0), position{}, rotationY(0.0f) {}
};

struct PACKET_C2S_ATTACK_EXECUTE
{
	PACKET_HEADER header;
	ZONE_VECTOR3 position;
	float rotationY;
	uint8_t hitIndex;
	PACKET_C2S_ATTACK_EXECUTE() : header{ sizeof(PACKET_C2S_ATTACK_EXECUTE), ePacketType::C2S_AttackExecute }, position{}, rotationY(0.0f), hitIndex(0) {}
};

struct PACKET_S2C_ATTACK_RESULT
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	uint64_t playerObjectNum;
	eZoneObjectType targetType;
	uint64_t targetObjectNum;
	uint64_t damage;
	uint64_t targetHP;
	uint64_t gainedExperience;
	PACKET_S2C_ATTACK_RESULT() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_ATTACK_RESULT) - sizeof(PROXY_HEADER), ePacketType::S2C_AttackResult }, result(eZoneResult::Fail), playerObjectNum(0), targetType(eZoneObjectType::Monster), targetObjectNum(0), damage(0), targetHP(0), gainedExperience(0) {}
};

struct PACKET_S2C_LEVEL_UP
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t playerObjectNum;
	uint64_t level;
	uint64_t experience;
	uint64_t requiredExperience;
	uint64_t maxHP;
	uint64_t hp;
	PACKET_S2C_LEVEL_UP() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_LEVEL_UP) - sizeof(PROXY_HEADER), ePacketType::S2C_LevelUp }, playerObjectNum(0), level(1), experience(0), requiredExperience(0), maxHP(0), hp(0) {}
};

struct PACKET_C2S_ATTACK_CANCEL
{
	PACKET_HEADER header;
	PACKET_C2S_ATTACK_CANCEL() : header{ sizeof(PACKET_C2S_ATTACK_CANCEL), ePacketType::C2S_AttackCancel } {}
};

struct PACKET_C2S_USE_POTION
{
	PACKET_HEADER header;
	ZONE_VECTOR3 position;
	float rotationY;
	PACKET_C2S_USE_POTION() : header{ sizeof(PACKET_C2S_USE_POTION), ePacketType::C2S_UsePotion }, position{}, rotationY(0.0f) {}
};

struct PACKET_S2C_USE_POTION
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	uint64_t playerObjectNum;
	uint64_t recoveryHP;
	uint64_t currentHP;
	uint64_t maxHP;
	PACKET_S2C_USE_POTION() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_USE_POTION) - sizeof(PROXY_HEADER), ePacketType::S2C_UsePotion }, result(eZoneResult::Fail), playerObjectNum(0), recoveryHP(0), currentHP(0), maxHP(0) {}
};

struct PACKET_C2S_CHANNEL_LIST
{
	PACKET_HEADER header;
	PACKET_C2S_CHANNEL_LIST() : header{ sizeof(PACKET_C2S_CHANNEL_LIST), ePacketType::C2S_ChannelList } {}
};

struct PACKET_S2C_CHANNEL_LIST
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t channelCount;
	CHANNEL_INFO channelList[1];
};

struct PACKET_C2S_CHANGE_CHANNEL
{
	PACKET_HEADER header;
	uint64_t channelId;
	PACKET_C2S_CHANGE_CHANNEL() : header{ sizeof(PACKET_C2S_CHANGE_CHANNEL), ePacketType::C2S_ChangeChannel }, channelId(0) {}
};

struct PACKET_S2C_CHANGE_CHANNEL
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	uint64_t channelId;
	uint64_t playerObjectNum;
	uint64_t experience;
	uint64_t requiredExperience;
	uint64_t objectCount;
	FIELD_OBJECT_INFO objectList[1];
};

struct PACKET_C2S_CHANGE_ZONE
{
	PACKET_HEADER header;
	uint64_t portalId;
	PACKET_C2S_CHANGE_ZONE() : header{ sizeof(PACKET_C2S_CHANGE_ZONE), ePacketType::C2S_ChangeZone }, portalId(0) {}
};

struct PACKET_S2C_CHANGE_ZONE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneResult result;
	uint64_t zoneId;
	ZONE_VECTOR3 position;
	PACKET_S2C_CHANGE_ZONE() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_CHANGE_ZONE) - sizeof(PROXY_HEADER), ePacketType::S2C_ChangeZone }, result(eZoneResult::Fail), zoneId(0), position{} {}
};

struct PACKET_S2C_FIELD_OBJECT_ENTER
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	FIELD_OBJECT_INFO objectInfo;
	PACKET_S2C_FIELD_OBJECT_ENTER() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_FIELD_OBJECT_ENTER) - sizeof(PROXY_HEADER), ePacketType::S2C_FieldObjectEnter }, objectInfo{} {}
};

struct PACKET_S2C_FIELD_OBJECT_LEAVE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	eZoneObjectType objectType;
	uint64_t objectNum;
	PACKET_S2C_FIELD_OBJECT_LEAVE() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_FIELD_OBJECT_LEAVE) - sizeof(PROXY_HEADER), ePacketType::S2C_FieldObjectLeave }, objectType(eZoneObjectType::Player), objectNum(0) {}
};

struct PACKET_S2C_MONSTER_STATE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t monsterObjectNum;
	eZoneObjectState state;
	ZONE_VECTOR3 position;
	ZONE_VECTOR3 destination;
	float rotationY;
	uint64_t targetPlayerObjectNum;
	uint64_t hp;
	uint64_t damage;
	uint64_t targetPlayerHP;
	PACKET_S2C_MONSTER_STATE() : proxyHeader{ NULL, NULL }, packetHeader{ sizeof(PACKET_S2C_MONSTER_STATE) - sizeof(PROXY_HEADER), ePacketType::S2C_MonsterState }, monsterObjectNum(0), state(eZoneObjectState::Idle), position{}, destination{}, rotationY(0.0f), targetPlayerObjectNum(0), hp(0), damage(0), targetPlayerHP(0) {}
};

struct PACKET_S2C_MONSTER_MOVE
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint64_t monsterObjectNum;
	ZONE_VECTOR3 position;
	float rotationY;
	float moveSpeed;
	uint64_t pathIndex;
	uint64_t pathCount;
	ZONE_VECTOR3 path[1];
};

struct PACKET_C2S_CHAT
{
	PACKET_HEADER header;
	uint8_t chatSize;
	char text[1];
};

struct PACKET_S2C_CHAT
{
	PROXY_HEADER proxyHeader;
	PACKET_HEADER packetHeader;
	uint8_t nameSize;
	uint8_t chatSize;
	char text[1];
};

struct PACKET_S2C_PLAYER_DEAD
{
	PROXY_HEADER	proxyHeader;
	PACKET_HEADER	packetHeader;
	uint64_t		playerObjectNum;
	ZONE_VECTOR3	position;
	float			rotationY;

	PACKET_S2C_PLAYER_DEAD() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_PLAYER_DEAD) - sizeof(PROXY_HEADER), ePacketType::S2C_PlayerDead },
		playerObjectNum(0), position{}, rotationY(0.0f) {}
};

struct PACKET_C2S_RESPAWN
{
	PACKET_HEADER header;

	PACKET_C2S_RESPAWN() : header{ sizeof(PACKET_C2S_RESPAWN), ePacketType::C2S_Respawn } {}
};

struct PACKET_S2C_RESPAWN
{
	PROXY_HEADER	proxyHeader;
	PACKET_HEADER	packetHeader;
	eZoneResult		result;
	uint64_t		zoneId;
	ZONE_VECTOR3	position;

	PACKET_S2C_RESPAWN() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_RESPAWN) - sizeof(PROXY_HEADER), ePacketType::S2C_Respawn },
		result(eZoneResult::Fail), zoneId(0), position{} {}
};

// 몬스터 상태 스냅샷과 별개인, 이번 한 번의 피격 결과
struct PACKET_S2C_PLAYER_HIT
{
	PROXY_HEADER	proxyHeader;
	PACKET_HEADER	packetHeader;
	uint64_t		playerObjectNum;
	uint64_t		monsterObjectNum;
	uint64_t		damage;
	uint64_t		hp;

	PACKET_S2C_PLAYER_HIT() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_PLAYER_HIT) - sizeof(PROXY_HEADER), ePacketType::S2C_PlayerHit },
		playerObjectNum(0), monsterObjectNum(0), damage(0), hp(0) {}
};

// =============================================

struct PACKET_C2S_DUMMY_PACKET8
{
	PACKET_HEADER header;
	uint64_t	  data;

	PACKET_C2S_DUMMY_PACKET8() :
		header{ sizeof(PACKET_C2S_DUMMY_PACKET8), ePacketType::C2S_DummyPacket8 } {}
};

struct PACKET_C2S_DUMMY_PACKET16
{
	PACKET_HEADER header;
	byte		  data[16];

	PACKET_C2S_DUMMY_PACKET16() :
		header{ sizeof(PACKET_C2S_DUMMY_PACKET16), ePacketType::C2S_DummyPacket16 } {}
};

struct PACKET_C2S_DUMMY_PACKET32
{
	PACKET_HEADER header;
	byte		  data[32];

	PACKET_C2S_DUMMY_PACKET32() :
		header{ sizeof(PACKET_C2S_DUMMY_PACKET32), ePacketType::C2S_DummyPacket32 } {}
};

struct PACKET_C2S_DUMMY_PACKET64
{
	PACKET_HEADER header;
	byte		  data[64];

	PACKET_C2S_DUMMY_PACKET64() :
		header{ sizeof(PACKET_C2S_DUMMY_PACKET64), ePacketType::C2S_DummyPacket64 } {}
};

struct PACKET_S2C_DUMMY_PACKET
{
	PROXY_HEADER  proxyHeader;
	PACKET_HEADER packetHeader;

	PACKET_S2C_DUMMY_PACKET() :
		proxyHeader{ NULL, NULL },
		packetHeader{ sizeof(PACKET_S2C_DUMMY_PACKET) - sizeof(PROXY_HEADER), ePacketType::S2C_DummyPacket } {}
};

#pragma pack()
