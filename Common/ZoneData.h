#pragma once
#include "ZONE.h"
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>

// 로그인/존서버가 동일한 존 설정 파일을 읽어서 사용
class CZoneData
{
private:
	std::vector<ZONE_INFO>			m_zoneList;
	std::vector<ZONE_PORTAL_INFO>	m_portalList;
	uint64_t						m_startZoneId = 0;

public:
	bool Load(const std::string& _path)
	{
		std::ifstream file(_path);
		if (!file.is_open()) return Error(_path, "file open");
		if (file.peek() == 0xEF) file.ignore(3);
		std::string name;
		uint64_t count = 0;
		if (!(file >> name >> count) || name != "ZoneCount" || count == 0 || count > 256) return Error(_path, "ZoneCount");
		std::vector<ZONE_INFO> zoneList(1);
		std::vector<ZONE_PORTAL_INFO> portalList;
		uint64_t startZoneId = 0;
		for (uint64_t i = 0; i < count; ++i)
		{
			ZONE_INFO info{};
			uint64_t start = 0;
			uint64_t active = 0;
			if (!(file >> name >> info.zoneId >> std::quoted(info.zoneName) >> std::quoted(info.sceneName)
				>> info.enterPosition.x >> info.enterPosition.y >> info.enterPosition.z >> start >> active)) return Error(_path, "Zone record");
			if (name != "Zone" || info.zoneId == 0 || info.zoneId > 65535 || start > 1 || active > 1) return Error(_path, "Zone values");
			if (info.zoneName.empty() || info.sceneName.empty() || !IsPosition(info.enterPosition)) return Error(_path, "Zone position/name");
			if (info.zoneId >= zoneList.size()) zoneList.resize(info.zoneId + 1);
			if (zoneList[info.zoneId].zoneId != 0) return Error(_path, "duplicate ZoneId");
			for (uint64_t j = 1; j < zoneList.size(); ++j)
			{
				if (zoneList[j].zoneId != 0 && zoneList[j].sceneName == info.sceneName) return Error(_path, "duplicate sceneName");
			}
			info.isStartZone = start != 0;
			info.isActive = active != 0;
			if (info.isStartZone)
			{
				if (!info.isActive || startZoneId != 0) return Error(_path, "start zone");
				startZoneId = info.zoneId;
			}
			zoneList[info.zoneId] = info;
		}
		if (startZoneId == 0) return Error(_path, "missing start zone");
		if (!(file >> name >> count) || name != "PortalCount" || count > 4096) return Error(_path, "PortalCount");
		for (uint64_t i = 0; i < count; ++i)
		{
			ZONE_PORTAL_INFO info{};
			uint64_t active = 0;
			if (!(file >> name >> info.sourceZoneId >> info.portalId >> info.targetZoneId
				>> info.enterPosition.x >> info.enterPosition.y >> info.enterPosition.z >> active)) return Error(_path, "Portal record");
			if (name != "Portal" || info.portalId == 0 || active > 1 || !IsPosition(info.enterPosition)) return Error(_path, "Portal values");
			if (info.sourceZoneId >= zoneList.size() || zoneList[info.sourceZoneId].zoneId == 0) return Error(_path, "portal source");
			if (info.targetZoneId >= zoneList.size() || zoneList[info.targetZoneId].zoneId == 0) return Error(_path, "portal target");
			if (active != 0 && !zoneList[info.targetZoneId].isActive) return Error(_path, "inactive portal target");
			for (uint64_t j = 0; j < portalList.size(); ++j)
			{
				if (portalList[j].sourceZoneId == info.sourceZoneId && portalList[j].portalId == info.portalId) return Error(_path, "duplicate portal");
			}
			info.isActive = active != 0;
			portalList.push_back(info);
		}
		if (file >> name) return Error(_path, "unexpected trailing data");
		m_zoneList.swap(zoneList);
		m_portalList.swap(portalList);
		m_startZoneId = startZoneId;
		return true;
	}

	bool FindZone(uint64_t _zoneId, ZONE_INFO& _info) const
	{
		_info = {};
		if (_zoneId >= m_zoneList.size() || m_zoneList[_zoneId].zoneId == 0) return false;
		_info = m_zoneList[_zoneId];
		return true;
	}

	bool FindPortal(uint64_t _zoneId, uint64_t _portalId, ZONE_PORTAL_INFO& _info) const
	{
		_info = {};
		for (uint64_t i = 0; i < m_portalList.size(); ++i)
		{
			if (m_portalList[i].sourceZoneId != _zoneId || m_portalList[i].portalId != _portalId) continue;
			_info = m_portalList[i];
			return true;
		}
		return false;
	}

	bool FindStartZone(ZONE_INFO& _info) const { return FindZone(m_startZoneId, _info); }

private:
	bool IsPosition(const ZONE_VECTOR3& _position) const
	{
		return std::isfinite(_position.x) && std::isfinite(_position.y) && std::isfinite(_position.z) &&
			_position.x >= 0.0f && _position.y == 0.0f && _position.z >= 0.0f;
	}
	bool Error(const std::string& _path, const char* _reason) const
	{
		std::cerr << "[ZoneData] " << _path << " : " << _reason << "\n";
		return false;
	}
};
