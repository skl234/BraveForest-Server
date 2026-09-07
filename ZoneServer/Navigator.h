#pragma once
#include <WinSock2.h>
#include "Navigation.h"

class CNavigator
{
private:
	CNavigation	m_navigation;

public:
	CNavigator() = default;
	~CNavigator() = default;

	std::vector<VECTOR3> Navigate(CCellGrid& _cellGrid, CCell* _start, CCell* _arrival);

private:
	//경로 최적화
	//시작점 및 각 꼭짓점과 꼭짓점 사이에 있는 점들은 삭제
	std::vector<VECTOR3> Optimization(std::vector<VECTOR3>& _path);
};
