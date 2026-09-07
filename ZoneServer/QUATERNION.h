#pragma once
#include "VECTOR3.h"

struct QUATERNION
{
	float	x;
	float	y;
	float	z;
	float	w;

	static QUATERNION LookRotation(const VECTOR3& dir)
	{
		VECTOR3 forward = dir.Normalize();
		float yaw = std::atan2(forward.x, forward.z); // Y축 회전

		float halfYaw = yaw * 0.5f;
		float sy = std::sin(halfYaw);
		float cy = std::cos(halfYaw);

		QUATERNION q;
		q.x = 0.0f;
		q.y = sy;
		q.z = 0.0f;
		q.w = cy;
		return q;
	}
};
