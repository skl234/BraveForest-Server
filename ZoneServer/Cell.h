#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <vector>
#include "VECTOR3.h"

class CCell
{
private:
	uint64_t	m_index = 0;
	bool		m_block = NULL;
	uint64_t	m_x = 0;
	uint64_t	m_z = 0;

	std::vector<CCell*> m_adjoingCellList; //(인접 Cell)
	//저장순서
	//↖ ↑ ↗ → ↘ ↓ ↙ ←
	//nullptr 값은 그 위치에 Cell이 없는것(가장자리부근의 cell)

	CCell* m_parent = nullptr; //부모 Cell(직전에 어디에서 왔는지)
	uint64_t m_g = 0; //시작점부터 현재Cell까지의 거리
	uint64_t m_h = 0; //현재Cell부터 도착점까지의 거리
	uint64_t m_f = 0; //총 예상거리
	uint64_t m_routePriority = 0; //경로 우선순위

public:
	CCell() = default;
	~CCell() = default;

	void Initialize(uint64_t _index, bool _block, uint64_t _x, uint64_t _z,
		std::vector<CCell*>& _adjoingCellList);
	bool IsBlock();
	void CleanPath();
	void SetPath(CCell* _parent, uint64_t _g, uint64_t _h, uint64_t _f, uint64_t _routePriority);

	uint64_t GetIndex();
	std::vector<CCell*>& GetAdjoingCellList();
	CCell* GetParent();
	uint64_t GetX();
	uint64_t GetZ();
	uint64_t GetG();
	uint64_t GetH();
	uint64_t GetF();
	uint64_t GetRoutePriority();

	VECTOR3 GetPosition();
};
