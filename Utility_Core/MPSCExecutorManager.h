#pragma once
#include <WinSock2.h>
#include <vector>
#include <memory>
#include "MPSCJobExecutor.h"

class CMPSCExecutorManager
{
private:
    std::vector<std::unique_ptr<CMPSCJobExecutor>> m_list;

public:
    CMPSCExecutorManager() = default;
    ~CMPSCExecutorManager() = default;

    bool Initialize(uint64_t _size);
    bool Start();
    void Stop();

    void Add(CJob* _job);
    void Add(CJob* _job, uint64_t _index);
    std::vector<JOB_TPS> GetTPSList();
};
