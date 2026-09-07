#pragma once
#include <WinSock2.h>
#include <cmath>

struct VECTOR3
{
	float	x;
	float	y;
	float	z;

	VECTOR3 operator-(const VECTOR3& other) const
	{
		return {x - other.x, y - other.y, z - other.z};
	}

	VECTOR3 operator+(const VECTOR3& other) const
	{
		return {x + other.x, y + other.y, z + other.z};
	}

	VECTOR3 operator*(float scalar) const
	{
		return {x * scalar, y * scalar, z * scalar};
	}
	bool operator==(const VECTOR3& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	float SqrMagnitude() const
	{
		return x * x + y * y + z * z;
	}

	float Magnitude() const
	{
		return std::sqrt(x * x + y * y + z * z);
	}

	VECTOR3 Normalize() const
	{
		float mag = Magnitude();
		if (mag == 0) return {0, 0, 0};
		return {x / mag, y / mag, z / mag};
	}

	static float Distance(const VECTOR3& a, const VECTOR3& b)
	{
		return (a - b).Magnitude();
	}
};
