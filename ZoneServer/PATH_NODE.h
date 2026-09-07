#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "Cell.h"

struct PATH_NODE
{
	uint64_t	f;
	CCell*		cell;

	bool operator<(const PATH_NODE& _other) const
	{
		if (f != _other.f) return f < _other.f;
		if (cell->GetH() != _other.cell->GetH()) return cell->GetH() < _other.cell->GetH();
		return cell->GetRoutePriority() < _other.cell->GetRoutePriority();
	}
	bool operator>(const PATH_NODE& _other) const
	{
		return _other < *this;
	}
	bool operator==(const PATH_NODE& _other) const
	{
		return cell == _other.cell;
	}
};
