#pragma once
#include <WinSock2.h>
#include <vector>
#include <set>

class CFieldObject;

class CSector
{
private:
	std::vector<CSector*>	m_adjoiningSectorList;
	std::set<CFieldObject*>	m_objectList;

public:
	CSector() = default;
	~CSector() = default;

	void SetAdjoiningSectorList(const std::vector<CSector*>& _adjoiningSectorList);

	void Enter(CFieldObject* _object);
	void Leave(CFieldObject* _object);
	bool IsContain(CFieldObject* _object);
	bool IsAdjoining(CSector* _sector);

	std::vector<CSector*>& GetAdjoiningSectorList();
	std::set<CFieldObject*>& GetObjectList();
};
