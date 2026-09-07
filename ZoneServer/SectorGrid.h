#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <vector>
#include "Sector.h"
#include "VECTOR3.h"

class CSectorGrid
{
private:
	std::vector<CSector>	m_sectorList;
	uint64_t				m_sectorWidth = 0;
	uint64_t				m_rows = 0;
	uint64_t				m_cols = 0;

public:
	CSectorGrid() = default;
	~CSectorGrid() = default;

	void Initialize(uint64_t _sectorWidth, uint64_t _rows, uint64_t _cols);
	CSector* GetSector(const VECTOR3& _position);
	CSector* GetSector(uint64_t _row, uint64_t _col);
	std::vector<CSector>& GetSectorList();

	uint64_t GetSectorWidth();
	uint64_t GetRows();
	uint64_t GetCols();

private:
	std::vector<CSector*> CreateAdjoiningSectorList(uint64_t _index);
};
