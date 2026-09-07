#pragma once
#include <functional>
#include "UtilityMacros.h"

class CJob
{
public:
	CJob() = default;
	virtual ~CJob() = default;

	virtual void Execute() = 0;
};
