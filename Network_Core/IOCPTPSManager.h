#pragma once
#include <WinSock2.h>
#include <vector>
#include "IOCP_TPS.h"
#include "OverlappedEx.h"
#include "../Utility_Core/UtilityMacros.h"

class CIOCPTPSManager
{
private:
	std::vector<IOCP_TPS> m_list;
	SRWLOCK				  m_lock;

public:
	CIOCPTPSManager();
	~CIOCPTPSManager() = default;

	DELETE_COPY_MOVE(CIOCPTPSManager);

	uint64_t RegisterIndex();
	void	 Update(uint64_t _index, eOverlappedType _type);

	std::vector<IOCP_TPS> GetList();
};
