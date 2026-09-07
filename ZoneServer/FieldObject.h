#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "FieldObjectType.h"
#include "VECTOR3.h"

class CField;
class CSector;
class CMemoryStream;

class CFieldObject
{
	friend class	CField;

protected:
	eFieldObjectType	m_objectType;
	uint64_t			m_num;
	bool				m_active;
	VECTOR3				m_position;
	float				m_rotationY;

	CField*		m_field;
	CSector*	m_sector;

public:
	CFieldObject(eFieldObjectType _type, uint64_t _num);
	virtual ~CFieldObject();

	eFieldObjectType GetObjectType();
	uint64_t GetNum();
	bool IsActive();
	VECTOR3& GetPosition();
	float GetRotationY();
	CField* GetField();
	CSector* GetSector();

	virtual void WriteInfo(CMemoryStream& _stream);
};
