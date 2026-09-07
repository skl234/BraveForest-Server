#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>
#include "CHARACTER.h"

constexpr uint64_t TOWN_ZONE_ID = 1;

enum class eZoneResult : uint8_t
{
	Success = 0,
	Fail,
	Full,
	InvalidCharacter,
	InvalidTarget,
	InvalidPosition,
};

enum class eZoneObjectType : uint8_t
{
	Player = 0,
	Monster,
};

enum class eZoneObjectState : uint8_t
{
	Idle = 0,
	Move,
	Chase,
	Attack,
	Return,
	Dead,
};

enum class eAttackType : uint8_t
{
	None = 0,
	BasicAttack,
	SkillA,
	SkillS,
	RapidFire,
};

#pragma pack(1)

struct ZONE_VECTOR3
{
	float x;
	float y;
	float z;
};

struct PLAYER_DATA
{
	uint64_t characterIndex;
	uint64_t accountIndex;
	eJobClass jobClass;
	uint64_t level;
	uint64_t experience;
	uint64_t maxHP;
	uint64_t hp;
	uint64_t zoneId;
	ZONE_VECTOR3 position;
	float rotationY;
};

struct FIELD_OBJECT_INFO
{
	eZoneObjectType objectType;
	uint64_t objectNum;
	uint64_t dataIndex;
	eJobClass jobClass;
	uint64_t level;
	uint64_t maxHP;
	uint64_t hp;
	ZONE_VECTOR3 position;
	float rotationY;
	eZoneObjectState state;
	uint8_t nameSize;
	char name[30];
};

struct ZONE_INFO
{
	uint64_t zoneId;
	std::string zoneName;
	std::string sceneName;
	ZONE_VECTOR3 enterPosition;
	bool isStartZone;
	bool isActive;
};

struct ZONE_PORTAL_INFO
{
	uint64_t portalId;
	uint64_t sourceZoneId;
	uint64_t targetZoneId;
	ZONE_VECTOR3 enterPosition;
	bool isActive;
};

struct CHANNEL_INFO
{
	uint64_t channelId;
	uint64_t playerCount;
	uint64_t maxPlayerCount;
	bool available;
};

#pragma pack()
