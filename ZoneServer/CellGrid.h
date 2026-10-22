#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <set>
#include "Cell.h"

class CCellGrid
{
private:
	std::vector<CCell>	m_cellList;

	uint64_t			m_rows = 0;
	uint64_t			m_cols = 0;

public:
	CCellGrid() = default;
	~CCellGrid() = default;

	void				Initialize(uint64_t _rows, uint64_t _cols);
	void				Initialize(uint64_t _rows, uint64_t _cols, const std::set<uint64_t>& _blockList);

	CCell*				GetCell(const VECTOR3& _position);
	std::vector<CCell>&	GetCellList();
	uint64_t			GetRows();
	uint64_t			GetCols();

	void				ShowMap();

private:
	std::vector<CCell*>	GetAdjoingCellList(uint64_t _index);
	bool				RandomBlock();
};
