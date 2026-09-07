#pragma once
#include <cstdint>
#include "ClassType.h"
#include "../Common/ZONE.h"

class CCombatDamageCalculator
{
public:
	static constexpr uint64_t	MAX_LEVEL = 20;

	static uint64_t CalculatePlayerDamage(eClassState _class, uint64_t _level, eAttackType _attackType);
	static uint64_t CalculateMonsterDamage(uint64_t _level);
};
