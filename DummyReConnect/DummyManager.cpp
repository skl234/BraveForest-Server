#include "DummyManager.h"
#include "../Utility_Core/Lock.h"

CDummyManager::CDummyManager()
{
	InitializeSRWLock(&m_lock);
}

CDummyManager::~CDummyManager()
{
	uint64_t size = m_list.size();
	for (uint64_t i = 0; i < size; ++i)
	{
		m_list[i]->Disconnect();
	}
}

void CDummyManager::Initialize(uint32_t _size)
{
	for (uint64_t i = 0; i < _size; ++i)
	{
		std::unique_ptr<CDummyClient> client = std::make_unique<CDummyClient>();

		m_list.emplace_back(std::move(client));
		m_pool.emplace_back(m_list.back().get());
	}
}

CDummyClient* CDummyManager::Acquire()
{
	CSRWExclusiveLock lock(m_lock);
	if (m_pool.empty()) return nullptr;

	CDummyClient* client = m_pool.front();
	m_pool.pop_front();

	return client;
}

void CDummyManager::Release(CDummyClient* _client)
{
	CSRWExclusiveLock lock(m_lock);
	m_pool.push_back(_client);
}

void CDummyManager::AllConnect(std::string _ip, uint16_t _port)
{
	int tCount = 6;
	std::vector<CConnectThread> tList;
	tList.reserve(tCount);

	for (int i = 0; i < tCount; ++i)
	{
		tList.emplace_back(m_list, _ip, _port, i, tCount);
	}
	for (int i = 0; i < tCount; ++i)
	{
		tList[i].Start();
	}
	for (int i = 0; i < tCount; ++i)
	{
		WaitForSingleObject(tList[i].GetHandle(), INFINITE);
	}
}

uint64_t CDummyManager::GetSize()
{
	return m_list.size();
}
