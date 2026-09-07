#include "Sector.h"

void CSector::SetAdjoiningSectorList(const std::vector<CSector*>& _adjoiningSectorList)
{
	m_adjoiningSectorList = _adjoiningSectorList;
}

void CSector::Enter(CFieldObject* _object)
{
	if (_object == nullptr) return;
	m_objectList.insert(_object);
}

void CSector::Leave(CFieldObject* _object)
{
	if (_object == nullptr) return;
	m_objectList.erase(_object);
}

bool CSector::IsContain(CFieldObject* _object)
{
	if (_object == nullptr) return false;
	return m_objectList.find(_object) != m_objectList.end();
}

bool CSector::IsAdjoining(CSector* _sector)
{
	if (_sector == nullptr) return false;
	if (this == _sector) return true;

	std::vector<CSector*>::iterator iter = m_adjoiningSectorList.begin();
	std::vector<CSector*>::iterator end = m_adjoiningSectorList.end();
	for (iter; iter != end; ++iter)
	{
		if ((*iter) == _sector) return true;
	}
	return false;
}

std::vector<CSector*>& CSector::GetAdjoiningSectorList()
{
	return m_adjoiningSectorList;
}

std::set<CFieldObject*>& CSector::GetObjectList()
{
	return m_objectList;
}
