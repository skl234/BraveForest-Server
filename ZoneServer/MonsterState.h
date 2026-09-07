#pragma once

enum class eMonsterState : unsigned short
{
	Idle = 0,
	Move,
	Chase,
	Attack,
	Return,
	Dead,
};
