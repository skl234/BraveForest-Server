#include "FieldObject.h"

CFieldObject::CFieldObject(eFieldObjectType _type, uint64_t _num) :
	m_objectType(_type),
	m_num(_num),
	m_active(false),
	m_position({0.0f, 0.0f, 0.0f}),
	m_rotationY(0.0f),
	m_field(nullptr),
	m_sector(nullptr)
{
}

CFieldObject::~CFieldObject()
{
}

eFieldObjectType CFieldObject::GetObjectType()
{
	return m_objectType;
}

uint64_t CFieldObject::GetNum()
{
	return m_num;
}

bool CFieldObject::IsActive()
{
	return m_active;
}

VECTOR3& CFieldObject::GetPosition()
{
	return m_position;
}

float CFieldObject::GetRotationY()
{
	return m_rotationY;
}

CField* CFieldObject::GetField()
{
	return m_field;
}

CSector* CFieldObject::GetSector()
{
	return m_sector;
}

void CFieldObject::WriteInfo(CMemoryStream& _stream)
{
	(void)_stream;
}
