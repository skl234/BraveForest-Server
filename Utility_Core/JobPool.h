#pragma once
#include <WinSock2.h>
#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include "Job.h"

class CJobPool
{
private:
    std::vector<std::unique_ptr<CJob>>  m_list;
    std::deque<CJob*>                   m_pool;
    SRWLOCK                             m_lock;

public:
    CJobPool();
    ~CJobPool() = default;

    void        Initialize(std::vector<std::unique_ptr<CJob>>&& _jobList);
    CJob*       Acquire();
    void        Release(CJob* _job);
    uint64_t    GetPoolSize();
};
