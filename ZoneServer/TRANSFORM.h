#pragma once
#include "VECTOR3.h"
#include "QUATERNION.h"

#pragma pack(4)
struct TRANSFORM
{
	VECTOR3		position;
	QUATERNION	rotation;
};
#pragma pack()
