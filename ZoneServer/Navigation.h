#pragma once
#include <WinSock2.h>
#include <set>
#include <queue>
#include <functional>
#include "PATH_NODE.h"
#include "CellGrid.h"
#include "VECTOR3.h"

class CNavigation
{
private:
	std::priority_queue<PATH_NODE, std::vector<PATH_NODE>, std::greater<PATH_NODE>>	m_openList;
	std::set<CCell*> m_openSet; //Open 상태 검사
	std::set<CCell*>	m_closeList;

public:
	CNavigation() = default;
	~CNavigation() = default;

	std::vector<VECTOR3> Navigate(CCellGrid& _cellGrid, CCell* _start, CCell* _arrival);

private:
	void Clear(std::vector<CCell>& _cellList);
};
