#include "CombatDamageCalculator.h"

uint64_t CCombatDamageCalculator::CalculatePlayerDamage(eClassState _class, uint64_t _level, eAttackType _attackType)
{
	if (_level == 0) _level = 1;
	if (_level > MAX_LEVEL) _level = MAX_LEVEL;

	uint64_t basicDamage = 18 + (_level - 1) * 4;
	if (_class == eClassState::Warrior) basicDamage = 22 + (_level - 1) * 5;

	if (_attackType == eAttackType::BasicAttack) return basicDamage;
	if (_attackType == eAttackType::SkillA)
	{
		if (_class == eClassState::Warrior) return basicDamage * 2;
		return basicDamage * 5 / 2;
	}
	if (_attackType == eAttackType::SkillS)
	{
		if (_class == eClassState::Warrior) return basicDamage * 3 / 2;
		return basicDamage * 3 / 10;
	}
	if (_attackType == eAttackType::RapidFire) return basicDamage * 3 / 10;
	return 0;
}

uint64_t CCombatDamageCalculator::CalculateMonsterDamage(uint64_t _level)
{
	if (_level == 0) _level = 1;
	if (_level > MAX_LEVEL) _level = MAX_LEVEL;
	return 5 + _level * 2;
}
