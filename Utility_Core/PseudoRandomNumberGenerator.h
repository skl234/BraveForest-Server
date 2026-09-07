#pragma once
#include <WinSock2.h>
#include <cstdint>
#include "UtilityMacros.h"

//의사난수
//의사난수중 Xorshift 을 이용한 방법
//이 방법에서는 seed가 0이면 안된다

class CPseudoRandomNumberGenerator
{
private:
	uint64_t m_state;

public:
	explicit CPseudoRandomNumberGenerator(uint64_t _seed = 0) noexcept;
	~CPseudoRandomNumberGenerator() = default;
	DELETE_COPY_MOVE(CPseudoRandomNumberGenerator);

	//0~max
	uint64_t GetRandomNumber(uint64_t _max) noexcept;
    uint64_t GetRandomNumber() noexcept;
};

using CPRNG = CPseudoRandomNumberGenerator;
