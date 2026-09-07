#include "IOCPTPSManager.h"
#include "../Utility_Core/LockGuard.h"

CIOCPTPSManager::CIOCPTPSManager()
{
	InitializeSRWLock(&m_lock);
}

uint64_t CIOCPTPSManager::RegisterIndex()
{
	CLockGuard guard(m_lock);
	uint64_t index = m_list.size();

	IOCP_TPS tps{ index, 0, };
	m_list.push_back(tps);

	return index;
}

void CIOCPTPSManager::Update(uint64_t _index, eOverlappedType _type)
{
	++m_list[_index].total;
	if (_type == eOverlappedType::Recv) ++m_list[_index].recvCount;
	if (_type == eOverlappedType::Send) ++m_list[_index].sendCount;
	if (_type == eOverlappedType::Accept) ++m_list[_index].acceptCount;
}

std::vector<IOCP_TPS> CIOCPTPSManager::GetList()
{
	std::vector<IOCP_TPS> temp = m_list;

	size_t size = m_list.size();
	for (size_t i = 0; i < size; ++i)
	{
		IOCP_TPS& tps = m_list[i];
		tps.total = 0;
		tps.recvCount = 0;
		tps.sendCount = 0;
		tps.acceptCount = 0;
	}

	return temp;
}
