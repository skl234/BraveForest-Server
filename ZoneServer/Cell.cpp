#include "Cell.h"

void CCell::Initialize(uint64_t _index, bool _block, uint64_t _x, uint64_t _z,
	std::vector<CCell*>& _adjoingCellList)
{
	m_index = _index;
	m_block = _block;
	m_x = _x;
	m_z = _z;

	m_adjoingCellList = _adjoingCellList;

	m_parent = nullptr;
	m_g = 0;
	m_h = 0;
	m_f = 0;
	m_routePriority = 0;
}

bool CCell::IsBlock()
{
	return m_block;
}

void CCell::CleanPath()
{
	m_parent = nullptr;
	m_g = 0;
	m_h = 0;
	m_f = 0;
	m_routePriority = 0;
}

void CCell::SetPath(CCell* _parent, uint64_t _g, uint64_t _h, uint64_t _f, uint64_t _routePriority)
{
	m_parent = _parent;
	m_g = _g;
	m_h = _h;
	m_f = _f;
	m_routePriority = _routePriority;
}

uint64_t CCell::GetIndex()
{
	return m_index;
}

std::vector<CCell*>& CCell::GetAdjoingCellList()
{
	return m_adjoingCellList;
}

CCell* CCell::GetParent()
{
	return m_parent;
}

uint64_t CCell::GetX()
{
	return m_x;
}

uint64_t CCell::GetZ()
{
	return m_z;
}

uint64_t CCell::GetG()
{
	return m_g;
}

uint64_t CCell::GetH()
{
	return m_h;
}

uint64_t CCell::GetF()
{
	return m_f;
}

uint64_t CCell::GetRoutePriority()
{
	return m_routePriority;
}

VECTOR3 CCell::GetPosition()
{
	return {static_cast<float>(m_x), 0.0f, static_cast<float>(m_z)};
}
