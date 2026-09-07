#include "JobPool.h"
#include "LockGuard.h"

CJobPool::CJobPool()
{
	InitializeSRWLock(&m_lock);
}

void CJobPool::Initialize(std::vector<std::unique_ptr<CJob>>&& _jobList)
{
    m_list = std::move(_jobList);

    uint64_t size = m_list.size();
    for (uint64_t i = 0; i < size; ++i)
    {
        CJob* job = m_list[i].get();
        m_pool.emplace_back(job);
    }
}

CJob* CJobPool::Acquire()
{
    CLockGuard lockGuard(m_lock);

    if (m_pool.empty()) return nullptr;

    CJob* job = m_pool.front();
    m_pool.pop_front();

    return job;
}

void CJobPool::Release(CJob* _job)
{
    if (_job == nullptr) return;

    CLockGuard lockGuard(m_lock);
    m_pool.push_back(_job);
}

uint64_t CJobPool::GetPoolSize()
{
    CLockGuard lockGuard(m_lock, eLockMode::Shared);

    return m_pool.size();
}
