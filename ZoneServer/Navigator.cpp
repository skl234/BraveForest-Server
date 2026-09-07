#include "Navigator.h"

std::vector<VECTOR3> CNavigator::Navigate(CCellGrid& _cellGrid, CCell* _start, CCell* _arrival)
{
	std::vector<VECTOR3> path = m_navigation.Navigate(_cellGrid, _start, _arrival);
	if (path.empty()) return path;

	std::vector<VECTOR3> optimizationPath = Optimization(path);
	if (!optimizationPath.empty()) optimizationPath.erase(optimizationPath.begin());

	return optimizationPath;
}

std::vector<VECTOR3> CNavigator::Optimization(std::vector<VECTOR3>& _path)
{
	std::vector<VECTOR3> optimizationPath;
	if (_path.size() < 2) return _path;

	optimizationPath.reserve(_path.size());

	optimizationPath.push_back(_path[0]); // 시작 지점

	VECTOR3 prevDir = _path[0] - _path[1];
	for (size_t i = 1; i < _path.size() - 1; ++i)
	{
		VECTOR3 dir = _path[i] - _path[i + 1];

		if (prevDir.x != dir.x || prevDir.z != dir.z)
		{
			optimizationPath.push_back(_path[i]);
			prevDir = dir;
		}
	}

	optimizationPath.push_back(_path[_path.size() - 1]);

	return optimizationPath;
}
