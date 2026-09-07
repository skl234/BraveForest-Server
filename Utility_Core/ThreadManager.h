#pragma once
#include <WinSock2.h>
#include <vector>
#include <memory>
#include <thread>
#include "UtilityMacros.h"

class CThreadManager
{
private:
	std::vector<std::thread> m_list;

public:
	CThreadManager() = default;
	~CThreadManager();

	DELETE_COPY_MOVE(CThreadManager);

	template<typename _T>
	void Add(_T _func);
	void AllJoin();
};

template<typename _T>
void CThreadManager::Add(_T _func)
{
	m_list.push_back(std::thread(_func));
}
