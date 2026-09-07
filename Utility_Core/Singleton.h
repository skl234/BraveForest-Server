#pragma once
#include "UtilityMacros.h"

template <typename _T>
class CSingleton
{
public:
	static _T& GetInstance()
	{
		static _T instance;
		return instance;
	}

protected:
	CSingleton() = default;
	~CSingleton() = default;

public:
	DELETE_COPY_MOVE(CSingleton);
};
