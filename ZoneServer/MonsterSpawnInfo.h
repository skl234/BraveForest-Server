#pragma once
#include "MonsterType.h"
#include "VECTOR3.h"

struct MONSTER_SPAWN_INFO
{
	eMonsterType	type;
	uint64_t		level;
	uint64_t		maxHP;
	uint64_t		experience;
	uint64_t		attackPower;
	bool			aggressive;
	bool			canWander = true;
	bool			canChase = true;
	bool			canAttack = true;
	float			detectionRange;
	float			attackRange;
	float			moveSpeed;
	uint64_t		attackIntervalMs;
	uint64_t		idleMinTimeMs = 1000;
	uint64_t		idleMaxTimeMs = 3000;
	uint64_t		wanderMinTimeMs = 2000;
	uint64_t		wanderMaxTimeMs = 5000;
	VECTOR3			spawnPosition;
	float			rotationY = 0.0f;
	float			wanderRadius;
	float			maxAggroRange;
	float			respawnRadius;
	uint64_t		respawnDelayMs;
};
