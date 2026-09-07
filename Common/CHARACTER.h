#pragma once
#include <WinSock2.h>
#include <cstdint>

enum class eJobClass : uint64_t
{
	Warrior = 0,
	Archer,
};

#pragma pack(1)

struct CHARACTER
{
	uint64_t  index;
	int64_t   nameLen;
	char      name[30];
	eJobClass jobClass;
	uint64_t  level;
	uint64_t  experience;
	uint64_t  lastZoneId;
	float     lastPositionX;
	float     lastPositionY;
	float     lastPositionZ;
};

#pragma pack()
