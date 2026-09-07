#include "CellGrid.h"
#include <iostream>
#include <iomanip>
#include <random>

void CCellGrid::Initialize(uint64_t _rows, uint64_t _cols)
{
	if (_rows == 0 || _cols == 0)
	{
		m_rows = 0;
		m_cols = 0;
		m_cellList.clear();
		return;
	}

	m_rows = _rows;
	m_cols = _cols;

	uint64_t listSize = m_rows * m_cols;
	m_cellList.resize(listSize);

	for (uint64_t i = 0; i < listSize; ++i)
	{
		std::vector<CCell*> adjoingCellList = GetAdjoingCellList(i);
		m_cellList[i].Initialize(i, RandomBlock(), i / m_cols, i % m_cols, adjoingCellList);
	}
}

void CCellGrid::Initialize(uint64_t _rows, uint64_t _cols, const std::set<uint64_t>& _blockList)
{
	if (_rows == 0 || _cols == 0)
	{
		m_rows = 0;
		m_cols = 0;
		m_cellList.clear();
		return;
	}

	m_rows = _rows;
	m_cols = _cols;

	uint64_t listSize = m_rows * m_cols;
	m_cellList.resize(listSize);

	for (uint64_t i = 0; i < listSize; ++i)
	{
		bool block = _blockList.find(i) != _blockList.end();
		std::vector<CCell*> adjoingCellList = GetAdjoingCellList(i);
		m_cellList[i].Initialize(i, block, i / m_cols, i % m_cols, adjoingCellList);
	}
}

void CCellGrid::ShowMap()
{
	uint64_t size = static_cast<uint64_t>(m_cellList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		if (i != 0 && i % m_cols == 0) std::cout << std::endl;

		if (m_cellList[i].IsBlock())
			std::cout << std::setw(4) << "/";
		else
			std::cout << std::setw(4) << m_cellList[i].GetIndex();
	}
	std::cout << std::endl;
}

std::vector<CCell>& CCellGrid::GetCellList()
{
	return m_cellList;
}

uint64_t CCellGrid::GetRows()
{
	return m_rows;
}

uint64_t CCellGrid::GetCols()
{
	return m_cols;
}

CCell* CCellGrid::GetCell(const VECTOR3& _position)
{
	if (_position.x < 0.0f || _position.z < 0.0f) return nullptr;
	uint64_t row = static_cast<uint64_t>(_position.x);
	uint64_t col = static_cast<uint64_t>(_position.z);
	if (row >= m_rows || col >= m_cols) return nullptr;
	return &m_cellList[row * m_cols + col];
}

std::vector<CCell*> CCellGrid::GetAdjoingCellList(uint64_t _index)
{
	std::vector<CCell*> adjCellList;
	adjCellList.reserve(8);
	uint64_t row = _index / m_cols;
	uint64_t col = _index % m_cols;

	uint64_t index;
	//좌상단
	if (row > 0 && col > 0)
	{
		index = (row - 1) * m_cols + (col - 1);
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//상단
	if (row > 0)
	{
		index = (row - 1) * m_cols + col;
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//우상단
	if (row > 0 && col < m_cols - 1)
	{
		index = (row - 1) * m_cols + (col + 1);
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//우
	if (col < m_cols - 1)
	{
		index = row * m_cols + (col + 1);
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//우하단
	if (row < m_rows - 1 && col < m_cols - 1)
	{
		index = (row + 1) * m_cols + (col + 1);
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//하단
	if (row < m_rows - 1)
	{
		index = (row + 1) * m_cols + col;
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//좌하단
	if (row < m_rows - 1 && col > 0)
	{
		index = (row + 1) * m_cols + (col - 1);
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	//좌
	if (col > 0)
	{
		index = row * m_cols + (col - 1);
		adjCellList.push_back(&m_cellList[index]);
	}
	else
		adjCellList.push_back(nullptr);

	return adjCellList;
}

bool CCellGrid::RandomBlock()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dist(1, 5);

	if (dist(gen) == 1) return true;
	return false;
}
