#include "MPSCExecutorManager.h"
#include "PseudoRandomNumberGenerator.h"

bool CMPSCExecutorManager::Initialize(uint64_t _size)
{
	m_list.reserve(_size);

	for (uint64_t i = 0; i < _size; ++i)
	{
		auto executor = std::make_unique<CMPSCJobExecutor>();
		if (!executor->Initialize()) return false;
		m_list.emplace_back(std::move(executor));
	}

	return true;
}

bool CMPSCExecutorManager::Start()
{
	uint64_t size = m_list.size();
	for (uint64_t i = 0; i < size; ++i)
	{
		if (!m_list[i]->Start()) return false;
	}

	return true;
}

void CMPSCExecutorManager::Stop()
{
	uint64_t size = m_list.size();
	for (uint64_t i = 0; i < size; ++i)
	{
		m_list[i]->Stop();
	}
}

void CMPSCExecutorManager::Add(CJob* _job)
{
	thread_local CPRNG rng(static_cast<uint64_t>(GetCurrentThreadId()));

	uint64_t size = m_list.size();
	uint64_t rn1 = rng.GetRandomNumber(size - 1);
	uint64_t rn2 = rng.GetRandomNumber(size - 1);

	if (rn1 == rn2)
		m_list[rn1]->Add(_job);
	else if (m_list[rn1]->GetSize() < m_list[rn2]->GetSize())
		m_list[rn1]->Add(_job);
	else
		m_list[rn2]->Add(_job);
}

void CMPSCExecutorManager::Add(CJob* _job, uint64_t _index)
{
	uint64_t size = m_list.size();
	uint64_t index = _index % size;
	m_list[index]->Add(_job);
}

std::vector<JOB_TPS> CMPSCExecutorManager::GetTPSList()
{
	uint64_t size = m_list.size();
	std::vector<JOB_TPS> list(size);
	for (uint64_t i = 0; i < size; ++i)
	{
		list[i] = m_list[i]->GetTPS();
	}

	return list;
}
