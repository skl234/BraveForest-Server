#include "CellMap.h"
#include <fstream>
#include <algorithm>
#include <cmath>

bool CCellMap::Load(const std::string& _path)
{
	std::ifstream file(_path);
	if (!file.is_open()) return false;
	if (file.peek() == 0xEF) file.ignore(3);
	std::string key;
	uint64_t value = 0;
	while (file >> key >> value)
	{
		if (key == "CellRows") m_rows = value;
		if (key == "CellCols") m_cols = value;
		if (key != "BlockCount") continue;
		if (m_rows == 0 || m_cols == 0 || m_rows > 4096 || m_cols > 4096) return false;
		m_blockList.assign(m_rows * m_cols, 0);
		if (value > m_blockList.size()) return false;
		for (uint64_t i = 0; i < value; ++i)
		{
			uint64_t index = 0;
			if (!(file >> index) || index >= m_blockList.size()) return false;
			m_blockList[index] = 1;
		}
		return true;
	}
	return false;
}

void CCellMap::AddPortal(float _x, float _z) { m_portalList.push_back({ _x, 0, _z }); }

float CCellMap::Distance(const ZONE_VECTOR3& _left, const ZONE_VECTOR3& _right)
{
	float x = _left.x - _right.x;
	float z = _left.z - _right.z;
	return std::sqrt(x * x + z * z);
}

bool CCellMap::IsWalkable(const ZONE_VECTOR3& _position) const
{
	if (!std::isfinite(_position.x) || !std::isfinite(_position.z)) return false;
	if (_position.x < 0 || _position.z < 0 || _position.x >= m_rows || _position.z >= m_cols) return false;
	// 서버 CellGrid와 동일: X가 row, Z가 col
	uint64_t index = static_cast<uint64_t>(_position.x) * m_cols + static_cast<uint64_t>(_position.z);
	return m_blockList[index] == 0;
}

bool CCellMap::IsAwayFromPortal(const ZONE_VECTOR3& _position, float _distance) const
{
	for (uint64_t i = 0; i < m_portalList.size(); ++i)
	{
		if (Distance(_position, m_portalList[i]) < _distance) return false;
	}
	return true;
}

bool CCellMap::IsInside(const ZONE_VECTOR3& _position, const ZONE_VECTOR3& _anchor) const
{
	return IsWalkable(_position) && Distance(_position, _anchor) <= 10.0001f && IsAwayFromPortal(_position, 4.0f);
}

ZONE_VECTOR3 CCellMap::GetCenter(uint64_t _index) const
{
	return { static_cast<float>(_index / m_cols) + .5f, 0, static_cast<float>(_index % m_cols) + .5f };
}

ZONE_VECTOR3 CCellMap::RandomDestination(const ZONE_VECTOR3& _anchor, std::mt19937& _random) const
{
	float angle = std::uniform_real_distribution<float>(0, 6.2831853f)(_random);
	float distance = std::uniform_real_distribution<float>(5, 10)(_random);
	ZONE_VECTOR3 point{ std::floor(_anchor.x + std::cos(angle) * distance) + .5f, 0, std::floor(_anchor.z + std::sin(angle) * distance) + .5f };
	return point;
}

bool CCellMap::CanStep(uint64_t _from, uint64_t _to) const
{
	if (m_blockList[_to] != 0) return false;
	uint64_t fromX = _from / m_cols;
	uint64_t fromZ = _from % m_cols;
	uint64_t toX = _to / m_cols;
	uint64_t toZ = _to % m_cols;
	// 대각선으로 장애물 모서리를 뚫지 않도록 양옆도 확인
	if (fromX != toX && fromZ != toZ)
	{
		if (m_blockList[fromX * m_cols + toZ] != 0 || m_blockList[toX * m_cols + fromZ] != 0) return false;
	}
	return true;
}

std::vector<ZONE_VECTOR3> CCellMap::FindPath(const ZONE_VECTOR3& _start, const ZONE_VECTOR3& _end, const ZONE_VECTOR3& _anchor)
{
	std::vector<ZONE_VECTOR3> path;
	if (!IsInside(_start, _anchor) || !IsInside(_end, _anchor)) return path;
	uint64_t start = static_cast<uint64_t>(_start.x) * m_cols + static_cast<uint64_t>(_start.z);
	uint64_t end = static_cast<uint64_t>(_end.x) * m_cols + static_cast<uint64_t>(_end.z);
	if (start == end) return path;
	m_parentList.assign(m_blockList.size(), UINT64_MAX);
	m_openList.clear();
	m_openList.push_back(start);
	m_parentList[start] = start;
	for (uint64_t i = 0; i < m_openList.size() && m_parentList[end] == UINT64_MAX; ++i)
	{
		uint64_t current = m_openList[i];
		for (int dx = -1; dx <= 1; ++dx)
		{
			for (int dz = -1; dz <= 1; ++dz)
			{
				if (dx == 0 && dz == 0) continue;
				int64_t x = static_cast<int64_t>(current / m_cols) + dx;
				int64_t z = static_cast<int64_t>(current % m_cols) + dz;
				if (x < 0 || z < 0 || x >= static_cast<int64_t>(m_rows) || z >= static_cast<int64_t>(m_cols)) continue;
				uint64_t next = static_cast<uint64_t>(x) * m_cols + static_cast<uint64_t>(z);
				if (m_parentList[next] != UINT64_MAX || !CanStep(current, next) || !IsInside(GetCenter(next), _anchor)) continue;
				m_parentList[next] = current;
				m_openList.push_back(next);
			}
		}
	}
	if (m_parentList[end] == UINT64_MAX) return path;
	// 재접속 위치가 셀 중앙이 아니면 현재 셀 안에서 먼저 정렬
	ZONE_VECTOR3 startCenter = GetCenter(start);
	if (Distance(_start, startCenter) > .001f)
	{
		if (!IsInside(startCenter, _anchor)) return path;
		path.push_back(startCenter);
	}
	std::vector<uint64_t> cells;
	for (uint64_t index = end; index != start; index = m_parentList[index]) cells.push_back(index);
	cells.push_back(start);
	std::reverse(cells.begin(), cells.end());
	// 시작점은 제외하고 꺾이는 지점과 도착점만 전달
	for (uint64_t i = 1; i + 1 < cells.size(); ++i)
	{
		ZONE_VECTOR3 previous = GetCenter(cells[i - 1]);
		ZONE_VECTOR3 current = GetCenter(cells[i]);
		ZONE_VECTOR3 next = GetCenter(cells[i + 1]);
		if (current.x - previous.x != next.x - current.x || current.z - previous.z != next.z - current.z) path.push_back(current);
	}
	path.push_back(GetCenter(end));
	return path;
}

std::vector<ZONE_VECTOR3> CCellMap::GetSpawnList(std::mt19937& _random)
{
	std::vector<ZONE_VECTOR3> result;
	for (uint64_t i = 0; i < m_blockList.size(); ++i)
	{
		ZONE_VECTOR3 position = GetCenter(i);
		// 배회 반경 10 + 포탈 진입 반경/여유 5
		if (!IsWalkable(position) || !IsAwayFromPortal(position, 15.0f)) continue;
		uint64_t routes = 0;
		for (uint64_t j = 0; j < 8; ++j)
		{
			float angle = static_cast<float>(j) * .78539816f;
			ZONE_VECTOR3 end{ std::floor(position.x + std::cos(angle) * 6) + .5f, 0, std::floor(position.z + std::sin(angle) * 6) + .5f };
			if (!FindPath(position, end, position).empty()) ++routes;
			if (routes >= 2) break;
		}
		if (routes >= 2) result.push_back(position);
	}
	std::shuffle(result.begin(), result.end(), _random);
	return result;
}
