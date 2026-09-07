#include "SectorGrid.h"

void CSectorGrid::Initialize(uint64_t _sectorWidth, uint64_t _rows, uint64_t _cols)
{
	m_sectorList.clear();

	if (_sectorWidth == 0 || _rows == 0 || _cols == 0)
	{
		m_sectorWidth = 0;
		m_rows = 0;
		m_cols = 0;
		return;
	}

	m_sectorWidth = _sectorWidth;
	m_rows = _rows;
	m_cols = _cols;

	uint64_t sectorSize = m_rows * m_cols;
	m_sectorList.resize(sectorSize);

	for (uint64_t i = 0; i < sectorSize; ++i)
	{
		std::vector<CSector*> adjoiningSectorList = CreateAdjoiningSectorList(i);
		m_sectorList[i].SetAdjoiningSectorList(adjoiningSectorList);
	}
}

CSector* CSectorGrid::GetSector(const VECTOR3& _position)
{
	if (m_sectorWidth == 0) return nullptr;
	if (_position.x < 0.0f || _position.z < 0.0f) return nullptr;

	uint64_t row = static_cast<uint64_t>(_position.x) / m_sectorWidth;
	uint64_t col = static_cast<uint64_t>(_position.z) / m_sectorWidth;
	return GetSector(row, col);
}

CSector* CSectorGrid::GetSector(uint64_t _row, uint64_t _col)
{
	if (_row >= m_rows || _col >= m_cols) return nullptr;
	return &m_sectorList[_row * m_cols + _col];
}

std::vector<CSector>& CSectorGrid::GetSectorList()
{
	return m_sectorList;
}

uint64_t CSectorGrid::GetSectorWidth()
{
	return m_sectorWidth;
}

uint64_t CSectorGrid::GetRows()
{
	return m_rows;
}

uint64_t CSectorGrid::GetCols()
{
	return m_cols;
}

std::vector<CSector*> CSectorGrid::CreateAdjoiningSectorList(uint64_t _index)
{
	std::vector<CSector*> adjoiningSectorList;
	adjoiningSectorList.reserve(8);

	uint64_t row = _index / m_cols;
	uint64_t col = _index % m_cols;

	if (row > 0 && col > 0)
		adjoiningSectorList.push_back(GetSector(row - 1, col - 1));
	else
		adjoiningSectorList.push_back(nullptr);

	if (row > 0)
		adjoiningSectorList.push_back(GetSector(row - 1, col));
	else
		adjoiningSectorList.push_back(nullptr);

	if (row > 0 && col + 1 < m_cols)
		adjoiningSectorList.push_back(GetSector(row - 1, col + 1));
	else
		adjoiningSectorList.push_back(nullptr);

	if (col + 1 < m_cols)
		adjoiningSectorList.push_back(GetSector(row, col + 1));
	else
		adjoiningSectorList.push_back(nullptr);

	if (row + 1 < m_rows && col + 1 < m_cols)
		adjoiningSectorList.push_back(GetSector(row + 1, col + 1));
	else
		adjoiningSectorList.push_back(nullptr);

	if (row + 1 < m_rows)
		adjoiningSectorList.push_back(GetSector(row + 1, col));
	else
		adjoiningSectorList.push_back(nullptr);

	if (row + 1 < m_rows && col > 0)
		adjoiningSectorList.push_back(GetSector(row + 1, col - 1));
	else
		adjoiningSectorList.push_back(nullptr);

	if (col > 0)
		adjoiningSectorList.push_back(GetSector(row, col - 1));
	else
		adjoiningSectorList.push_back(nullptr);

	return adjoiningSectorList;
}
