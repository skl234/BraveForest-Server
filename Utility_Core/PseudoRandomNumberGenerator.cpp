#include "PseudoRandomNumberGenerator.h"

CPseudoRandomNumberGenerator::CPseudoRandomNumberGenerator(uint64_t _seed) noexcept
{
	if (_seed) m_state = _seed;
	else m_state = 88172645463325252ull;
}

uint64_t CPseudoRandomNumberGenerator::GetRandomNumber(uint64_t _max) noexcept
{
    uint64_t num = GetRandomNumber();
    return num % (_max + 1);
}

uint64_t CPseudoRandomNumberGenerator::GetRandomNumber() noexcept
{
    uint64_t x = m_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    m_state = x;
    return x;
}
