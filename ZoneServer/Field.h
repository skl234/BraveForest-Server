#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include "CellGrid.h"
#include "SectorGrid.h"
#include "FieldObject.h"
#include "MonsterSpawnInfo.h"

class CField
{
private:
	uint64_t	m_widthX;
	uint64_t	m_widthZ;
	CCellGrid	m_cellGrid;
	CSectorGrid	m_sectorGrid;

public:
	CField();
	~CField() = default;

	bool Initialize(const std::string& _infoPath, std::vector<MONSTER_SPAWN_INFO>& _spawnInfoList, bool _loadMonster = true);

	bool Enter(CFieldObject* _object);
	void Leave(CFieldObject* _object);
	bool Move(CFieldObject* _object, const VECTOR3& _position);

	CCellGrid* GetCellGrid();
	CSectorGrid* GetSectorGrid();
	uint64_t GetWidth_X();
	uint64_t GetWidth_Z();
	bool IsWalkable(const VECTOR3& _position);
	bool IsClearLine(const VECTOR3& _start, const VECTOR3& _end);

private:
	std::vector<CSector*> CreateVisibleSectorList(CSector* _sector);
	bool IsValidPosition(const VECTOR3& _position);
	bool IsContainSector(std::vector<CSector*>& _sectorList, CSector* _sector);
	void EnterVisibleSector(CFieldObject* _object, std::vector<CSector*>& _sectorList);
	void LeaveVisibleSector(CFieldObject* _object, std::vector<CSector*>& _sectorList);
};
