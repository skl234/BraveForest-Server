#include "Navigation.h"
#include <algorithm>
#include <cmath>

std::vector<VECTOR3> CNavigation::Navigate(CCellGrid& _cellGrid, CCell* _start, CCell* _arrival)
{
	std::vector<VECTOR3> path;
	if (_start == nullptr || _arrival == nullptr) return path;

	std::vector<CCell>& cellList = _cellGrid.GetCellList();
	uint64_t cellListSize = static_cast<uint64_t>(cellList.size());
	uint64_t startIndex = _start->GetIndex();
	uint64_t arrivalIndex = _arrival->GetIndex();
	if (startIndex >= cellListSize) return path;
	if (arrivalIndex >= cellListSize) return path;
	if (&cellList[startIndex] != _start || &cellList[arrivalIndex] != _arrival) return path;

	// 시작과 도착 index Block 검사
	if (_start->IsBlock() || _arrival->IsBlock()) return path;

	// 탐색전 경로정보 초기화
	Clear(cellList);

	// 시작 Cell 과 도착 Cell
	CCell* startCell = _start;
	CCell* arrivalCell = _arrival;

	//현재 탐색 중인 Cell과 검사중인 주변 Cell
	CCell* parentCell = nullptr;
	CCell* adjoiningCell = nullptr;

	// 비용 값
	PATH_NODE node;
	uint64_t g;
	uint64_t h;
	uint64_t f;
	uint64_t distanceX;
	uint64_t distanceZ;

	// 시작 Cell 값 설정
	if (startCell->GetX() >= arrivalCell->GetX())
	{
		distanceX = startCell->GetX() - arrivalCell->GetX();
	}
	else
	{
		distanceX = arrivalCell->GetX() - startCell->GetX();
	}

	if (startCell->GetZ() >= arrivalCell->GetZ())
	{
		distanceZ = startCell->GetZ() - arrivalCell->GetZ();
	}
	else
	{
		distanceZ = arrivalCell->GetZ() - startCell->GetZ();
	}

	h = 10 * (distanceX + distanceZ);
	g = 0;
	f = g + h;
	startCell->SetPath(nullptr, g, h, f, 0);
	m_openList.push({f, startCell});
	m_openSet.insert(startCell);

	bool findPath = false;
	while (!m_openList.empty())
	{
		node = m_openList.top();
		m_openList.pop();
		m_openSet.erase(node.cell);

		// 현재 노드검사중인 노드의 Cell을 부모로
		parentCell = node.cell;

		// 이미 탐색해본 Cell인지 검사
		if (m_closeList.find(parentCell) != m_closeList.end()) continue;

		// 탐색한 Cell목록에 넣기
		m_closeList.insert(parentCell);

		// 탐색 완료
		if (parentCell == arrivalCell)
		{
			findPath = true;
			break;
		}

		// 주변 Cell 검사
		std::vector<CCell*>& adjoiningCellList = parentCell->GetAdjoingCellList();

		// 주변 8방향 검사
		for (uint64_t i = 0; i < 8; ++i)
		{
			adjoiningCell = adjoiningCellList[i];

			// 가장자리 Cell (nullptr로 막혀있음)
			if (adjoiningCell == nullptr) continue;
			// 이동 불가능 지역
			if (adjoiningCell->IsBlock()) continue;
			// 대각선 양 옆이 막혀 있으면 그 모서리 사이로 통과하지 않음
			if (i % 2 == 0)
			{
				CCell* left = adjoiningCellList[(i + 7) % 8];
				CCell* right = adjoiningCellList[(i + 1) % 8];
				if (left == nullptr || right == nullptr || left->IsBlock() || right->IsBlock()) continue;
			}
			// 이미 탐색해본 Cell
			if (m_closeList.find(adjoiningCell) != m_closeList.end()) continue;

			// Cell 탐색 순서
			// 0		1		2		3		4		5		6		7
			// 좌상단	상단		우상단	우측		우하단	하단		좌하단	좌측
			// 대각(짝수) 비용 14
			// 직선(홀수) 비용 10
			if (i % 2 == 0)
				g = parentCell->GetG() + 14;
			else
				g = parentCell->GetG() + 10;

			// 검사중인 주변 Cell에서 도착 Cell까지의 거리 계산
			if (adjoiningCell->GetX() >= arrivalCell->GetX())
			{
				distanceX = adjoiningCell->GetX() - arrivalCell->GetX();
			}
			else
			{
				distanceX = arrivalCell->GetX() - adjoiningCell->GetX();
			}

			if (adjoiningCell->GetZ() >= arrivalCell->GetZ())
			{
				distanceZ = adjoiningCell->GetZ() - arrivalCell->GetZ();
			}
			else
			{
				distanceZ = arrivalCell->GetZ() - adjoiningCell->GetZ();
			}

			h = 10 * (distanceX + distanceZ);

			// 현재 비용 + 예상 비용 = 총 예상 비용
			f = g + h;

			// 이미 OpenList에 포함되어 있으면 이미 기존이 더 빠를경우 더 추가x
			bool isOpen = m_openSet.find(adjoiningCell) != m_openSet.end();
			if (isOpen && g >= adjoiningCell->GetG()) continue;

			// 처음 발견했거나 기존보다 빠른 경로라면 추가
			// 새로 추가된 Node가 나중에 검사를 마치고 Close목록에 들어간다면
			// 이미 기존에 있던 Node(더 긴 경로로 계산된 Cell) 은 Close에서 이미 걸러주기 때문에 추가로 검사하지 않는다
			adjoiningCell->SetPath(parentCell, g, h, f, i);
			m_openList.push({f, adjoiningCell});
			if (!isOpen) m_openSet.insert(adjoiningCell);
		}
	}

	// 경로....없다?
	if (!findPath) return path;

	// 경로 역순 저장 (그냥 저장하면 도착지 > 시작지 가 된다)
	CCell* lastCell = arrivalCell;
	while (lastCell != nullptr)
	{
		path.push_back(lastCell->GetPosition());
		lastCell = lastCell->GetParent();
	}
	std::reverse(path.begin(), path.end());
	return path;
}

void CNavigation::Clear(std::vector<CCell>& _cellList)
{
	m_openList = std::priority_queue<PATH_NODE, std::vector<PATH_NODE>, std::greater<PATH_NODE>>();
	m_openSet.clear();
	m_closeList.clear();

	uint64_t size = static_cast<uint64_t>(_cellList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		_cellList[i].CleanPath();
	}
}
