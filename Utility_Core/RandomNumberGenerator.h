#pragma once
#include <memory>

class CRandomNumberGenerator
{
public:
	static int Genarate_int(int _min, int _max);
	static float Genarate_float(float _min, float _max);
	static int64_t Genarate_int64(int64_t _min, int64_t _max);
};
