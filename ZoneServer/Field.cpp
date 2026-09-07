#include "Field.h"
#include "Player.h"
#include <fstream>
#include <set>
#include <cmath>
#include <cfloat>

CField::CField() :
	m_widthX(0),
	m_widthZ(0)
{
}

bool CField::Initialize(const std::string& _infoPath, std::vector<MONSTER_SPAWN_INFO>& _spawnInfoList, bool _loadMonster)
{
	_spawnInfoList.clear();

	std::ifstream file(_infoPath);
	if (!file.is_open()) return false;

	uint64_t cellRows = 0;
	uint64_t cellCols = 0;
	uint64_t sectorWidth = 0;
	uint64_t sectorRows = 0;
	uint64_t sectorCols = 0;
	uint64_t blockCount = 0;
	uint64_t monsterCount = 0;
	std::set<uint64_t> blockList;
	std::string name;

	file >> name >> m_widthX;
	if (name != "WidthX") return false;
	file >> name >> m_widthZ;
	if (name != "WidthZ") return false;
	file >> name >> cellRows;
	if (name != "CellRows") return false;
	file >> name >> cellCols;
	if (name != "CellCols") return false;
	file >> name >> sectorWidth;
	if (name != "SectorWidth") return false;
	file >> name >> sectorRows;
	if (name != "SectorRows") return false;
	file >> name >> sectorCols;
	if (name != "SectorCols") return false;
	file >> name >> blockCount;
	if (name != "BlockCount") return false;
	if (file.fail()) return false;
	if (cellRows == 0 || cellCols == 0 || cellRows > UINT64_MAX / cellCols) return false;
	if (blockCount > cellRows * cellCols) return false;

	for (uint64_t i = 0; i < blockCount; ++i)
	{
		uint64_t blockIndex = 0;
		file >> blockIndex;
		if (file.fail()) return false;
		if (blockIndex >= cellRows * cellCols) return false;
		if (!blockList.insert(blockIndex).second) return false;
	}

	if (m_widthX == 0 || m_widthZ == 0) return false;
	if (cellRows == 0 || cellCols == 0) return false;
	if (sectorWidth == 0 || sectorRows == 0 || sectorCols == 0) return false;
	if (sectorRows > UINT64_MAX / sectorCols) return false;
	if (sectorWidth > UINT64_MAX / sectorRows || sectorWidth > UINT64_MAX / sectorCols) return false;
	// 셀 한 칸은 1 단위. 맵 범위와 셀 범위가 다르면 사용하지 않음
	if (cellRows != m_widthX || cellCols != m_widthZ) return false;
	if (sectorWidth * sectorRows < m_widthX) return false;
	if (sectorWidth * sectorCols < m_widthZ) return false;

	m_cellGrid.Initialize(cellRows, cellCols, blockList);
	m_sectorGrid.Initialize(sectorWidth, sectorRows, sectorCols);

	// 몬스터 설정은 DB, 배치는 별도 스폰 파일 사용
	if (!_loadMonster) return true;

	file >> name >> monsterCount;
	if (name != "MonsterCount") return false;
	if (file.fail() || monsterCount > 512) return false;

	for (uint64_t i = 0; i < monsterCount; ++i)
	{
		uint64_t type = 0;
		MONSTER_SPAWN_INFO spawnInfo = {};
		float respawnDelaySeconds = 0.0f;

		file >> name >> type >> spawnInfo.spawnPosition.x >> spawnInfo.spawnPosition.z >> spawnInfo.wanderRadius >> spawnInfo.maxAggroRange >> spawnInfo.respawnRadius >> respawnDelaySeconds;
		if (name != "Monster") return false;
		if (file.fail()) return false;
		if (type > static_cast<uint64_t>(eMonsterType::Rat)) return false;
		if (!IsValidPosition(spawnInfo.spawnPosition)) return false;
		if (spawnInfo.wanderRadius < 0.0f) return false;
		if (spawnInfo.maxAggroRange < 0.0f) return false;
		if (spawnInfo.respawnRadius < 0.0f) return false;
		if (respawnDelaySeconds < 0.0f) return false;
		if (spawnInfo.wanderRadius > spawnInfo.maxAggroRange) return false;
		if (spawnInfo.respawnRadius > spawnInfo.maxAggroRange) return false;

		spawnInfo.type = static_cast<eMonsterType>(type);
		spawnInfo.level = 1;
		spawnInfo.maxHP = 100;
		spawnInfo.experience = 10;
		spawnInfo.aggressive = false;
		spawnInfo.detectionRange = 5.0f;
		spawnInfo.attackRange = 1.5f;
		spawnInfo.moveSpeed = 3.0f;
		spawnInfo.attackIntervalMs = 2000;
		spawnInfo.respawnDelayMs = static_cast<uint64_t>(respawnDelaySeconds * 1000.0f);
		spawnInfo.spawnPosition.y = 0.0f;
		_spawnInfoList.push_back(spawnInfo);
	}

	return true;
}

bool CField::Enter(CFieldObject* _object)
{
	if (_object == nullptr) return false;
	if (_object->m_field != nullptr || _object->m_sector != nullptr) return false;

	_object->m_position.y = 0.0f;
	if (!IsValidPosition(_object->m_position)) return false;
	CSector* sector = m_sectorGrid.GetSector(_object->m_position);
	if (sector == nullptr) return false;

	_object->m_field = this;
	_object->m_sector = sector;
	_object->m_active = true;

	std::vector<CSector*> visibleSectorList = CreateVisibleSectorList(sector);
	EnterVisibleSector(_object, visibleSectorList);
	sector->Enter(_object);
	return true;
}

void CField::Leave(CFieldObject* _object)
{
	if (_object == nullptr) return;
	if (_object->m_field != this || _object->m_sector == nullptr) return;

	std::vector<CSector*> visibleSectorList = CreateVisibleSectorList(_object->m_sector);
	LeaveVisibleSector(_object, visibleSectorList);

	_object->m_sector->Leave(_object);
	_object->m_active = false;
	_object->m_sector = nullptr;
	_object->m_field = nullptr;
}

bool CField::Move(CFieldObject* _object, const VECTOR3& _position)
{
	if (_object == nullptr) return false;
	if (_object->m_field != this || _object->m_sector == nullptr) return false;

	VECTOR3 position = _position;
	position.y = 0.0f;
	if (!IsValidPosition(position)) return false;
	CSector* oldSector = _object->m_sector;
	CSector* newSector = m_sectorGrid.GetSector(position);
	if (newSector == nullptr) return false;

	_object->m_position = position;
	if (oldSector == newSector) return true;

	std::vector<CSector*> oldVisibleSectorList = CreateVisibleSectorList(oldSector);
	std::vector<CSector*> newVisibleSectorList = CreateVisibleSectorList(newSector);
	std::vector<CSector*> leaveSectorList;
	std::vector<CSector*> enterSectorList;

	uint64_t oldSize = static_cast<uint64_t>(oldVisibleSectorList.size());
	for (uint64_t i = 0; i < oldSize; ++i)
	{
		CSector* sector = oldVisibleSectorList[i];
		if (IsContainSector(newVisibleSectorList, sector)) continue;
		leaveSectorList.push_back(sector);
	}

	uint64_t newSize = static_cast<uint64_t>(newVisibleSectorList.size());
	for (uint64_t i = 0; i < newSize; ++i)
	{
		CSector* sector = newVisibleSectorList[i];
		if (IsContainSector(oldVisibleSectorList, sector)) continue;
		enterSectorList.push_back(sector);
	}

	LeaveVisibleSector(_object, leaveSectorList);
	oldSector->Leave(_object);
	newSector->Enter(_object);
	_object->m_sector = newSector;
	EnterVisibleSector(_object, enterSectorList);
	return true;
}

CCellGrid* CField::GetCellGrid()
{
	return &m_cellGrid;
}

CSectorGrid* CField::GetSectorGrid()
{
	return &m_sectorGrid;
}

uint64_t CField::GetWidth_X()
{
	return m_widthX;
}

uint64_t CField::GetWidth_Z()
{
	return m_widthZ;
}

bool CField::IsWalkable(const VECTOR3& _position)
{
	return IsValidPosition(_position);
}

bool CField::IsClearLine(const VECTOR3& _start, const VECTOR3& _end)
{
	if (!IsWalkable(_start) || !IsWalkable(_end)) return false;
	float x = std::floor(_start.x), z = std::floor(_start.z);
	float endX = std::floor(_end.x), endZ = std::floor(_end.z);
	float dx = _end.x - _start.x, dz = _end.z - _start.z;
	float stepX = 0, stepZ = 0, nextX = FLT_MAX, nextZ = FLT_MAX, deltaX = FLT_MAX, deltaZ = FLT_MAX;
	if (dx > 0)
	{
		stepX = 1;
		nextX = (x + 1 - _start.x) / dx;
		deltaX = 1 / dx;
	}
	if (dx < 0)
	{
		stepX = -1;
		nextX = (x - _start.x) / dx;
		deltaX = -1 / dx;
	}
	if (dz > 0)
	{
		stepZ = 1;
		nextZ = (z + 1 - _start.z) / dz;
		deltaZ = 1 / dz;
	}
	if (dz < 0)
	{
		stepZ = -1;
		nextZ = (z - _start.z) / dz;
		deltaZ = -1 / dz;
	}
	// 선분이 지나는 셀을 순서대로 확인. 대각 모서리 양쪽도 포함
	while (x != endX || z != endZ)
	{
		// 목적지가 셀 경계에 걸려도 이미 도착한 축을 한 셀 더 넘기지 않음
		if (x == endX) nextX = FLT_MAX;
		if (z == endZ) nextZ = FLT_MAX;
		if (std::fabs(nextX - nextZ) < 0.00001f)
		{
			if (!IsWalkable({x + stepX + 0.5f, 0, z + 0.5f}) || !IsWalkable({x + 0.5f, 0, z + stepZ + 0.5f})) return false;
			x += stepX;
			z += stepZ;
			nextX += deltaX;
			nextZ += deltaZ;
		}
		else if (nextX < nextZ)
		{
			x += stepX;
			nextX += deltaX;
		}
		else
		{
			z += stepZ;
			nextZ += deltaZ;
		}
		if (!IsWalkable({x + 0.5f, 0, z + 0.5f})) return false;
	}
	return true;
}

std::vector<CSector*> CField::CreateVisibleSectorList(CSector* _sector)
{
	std::vector<CSector*> sectorList;
	if (_sector == nullptr) return sectorList;

	sectorList.push_back(_sector);
	std::vector<CSector*>& adjoiningSectorList = _sector->GetAdjoiningSectorList();
	uint64_t size = static_cast<uint64_t>(adjoiningSectorList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		if (adjoiningSectorList[i] == nullptr) continue;
		sectorList.push_back(adjoiningSectorList[i]);
	}
	return sectorList;
}

bool CField::IsValidPosition(const VECTOR3& _position)
{
	if (!std::isfinite(_position.x) || !std::isfinite(_position.z)) return false;
	if (_position.x < 0.0f || _position.z < 0.0f) return false;
	if (_position.x >= static_cast<float>(m_widthX)) return false;
	if (_position.z >= static_cast<float>(m_widthZ)) return false;
	uint64_t row = static_cast<uint64_t>(std::floor(_position.x));
	uint64_t col = static_cast<uint64_t>(std::floor(_position.z));
	if (row >= m_cellGrid.GetRows() || col >= m_cellGrid.GetCols()) return false;
	uint64_t index = row * m_cellGrid.GetCols() + col;
	if (m_cellGrid.GetCellList()[index].IsBlock()) return false;
	return true;
}

bool CField::IsContainSector(std::vector<CSector*>& _sectorList, CSector* _sector)
{
	uint64_t size = static_cast<uint64_t>(_sectorList.size());
	for (uint64_t i = 0; i < size; ++i)
	{
		if (_sectorList[i] == _sector) return true;
	}
	return false;
}

void CField::EnterVisibleSector(CFieldObject* _object, std::vector<CSector*>& _sectorList)
{
	CPlayer* player = nullptr;
	if (_object->GetObjectType() == eFieldObjectType::Player)
	{
		player = static_cast<CPlayer*>(_object);
	}

	uint64_t sectorSize = static_cast<uint64_t>(_sectorList.size());
	for (uint64_t i = 0; i < sectorSize; ++i)
	{
		std::set<CFieldObject*>& objectList = _sectorList[i]->GetObjectList();
		std::set<CFieldObject*>::iterator iter = objectList.begin();
		std::set<CFieldObject*>::iterator end = objectList.end();
		for (iter; iter != end; ++iter)
		{
			CFieldObject* visibleObject = *iter;
			if (visibleObject == _object) continue;

			if (player != nullptr) player->EnterView(visibleObject);
			if (visibleObject->GetObjectType() != eFieldObjectType::Player) continue;

			CPlayer* visiblePlayer = static_cast<CPlayer*>(visibleObject);
			visiblePlayer->EnterView(_object);
		}
	}
}

void CField::LeaveVisibleSector(CFieldObject* _object, std::vector<CSector*>& _sectorList)
{
	CPlayer* player = nullptr;
	if (_object->GetObjectType() == eFieldObjectType::Player)
	{
		player = static_cast<CPlayer*>(_object);
	}

	uint64_t sectorSize = static_cast<uint64_t>(_sectorList.size());
	for (uint64_t i = 0; i < sectorSize; ++i)
	{
		std::set<CFieldObject*>& objectList = _sectorList[i]->GetObjectList();
		std::set<CFieldObject*>::iterator iter = objectList.begin();
		std::set<CFieldObject*>::iterator end = objectList.end();
		for (iter; iter != end; ++iter)
		{
			CFieldObject* visibleObject = *iter;
			if (visibleObject == _object) continue;

			if (player != nullptr) player->LeaveView(visibleObject);
			if (visibleObject->GetObjectType() != eFieldObjectType::Player) continue;

			CPlayer* visiblePlayer = static_cast<CPlayer*>(visibleObject);
			visiblePlayer->LeaveView(_object);
		}
	}
}
