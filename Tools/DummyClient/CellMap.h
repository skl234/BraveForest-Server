#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <random>
#include "../../Common/ZONE.h"

class CCellMap
{
private:
	uint64_t				m_rows = 0;
	uint64_t				m_cols = 0;
	std::vector<uint8_t>		m_blockList;
	std::vector<ZONE_VECTOR3> m_portalList;
	std::vector<uint64_t>		m_parentList;
	std::vector<uint64_t>		m_openList;

public:
	bool Load(const std::string& _path);
	void AddPortal(float _x, float _z);
	bool IsWalkable(const ZONE_VECTOR3& _position) const;
	bool IsAwayFromPortal(const ZONE_VECTOR3& _position, float _distance) const;
	bool IsInside(const ZONE_VECTOR3& _position, const ZONE_VECTOR3& _anchor) const;
	ZONE_VECTOR3 RandomDestination(const ZONE_VECTOR3& _anchor, std::mt19937& _random) const;
	std::vector<ZONE_VECTOR3> FindPath(const ZONE_VECTOR3& _start, const ZONE_VECTOR3& _end, const ZONE_VECTOR3& _anchor);
	std::vector<ZONE_VECTOR3> GetSpawnList(std::mt19937& _random);
	static float Distance(const ZONE_VECTOR3& _left, const ZONE_VECTOR3& _right);

private:
	ZONE_VECTOR3 GetCenter(uint64_t _index) const;
	bool CanStep(uint64_t _from, uint64_t _to) const;
};
