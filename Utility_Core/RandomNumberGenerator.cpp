#include "RandomNumberGenerator.h"
#include <random>
#include <algorithm>

int CRandomNumberGenerator::Genarate_int(int _min, int _max)
{
    if (_min > _max) std::swap(_min, _max);

    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(_min, _max);
    return dist(rng);
}

float CRandomNumberGenerator::Genarate_float(float _min, float _max)
{
    if (_min > _max) std::swap(_min, _max);

    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(_min, _max);
    return dist(rng);
}

int64_t CRandomNumberGenerator::Genarate_int64(int64_t _min, int64_t _max)
{
    if (_min > _max) std::swap(_min, _max);

    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int64_t> dist(_min, _max);
    return dist(rng);
}
