#pragma once
#include <WinSock2.h>
#include <deque>
#include <memory>
#include "LockGuard.h"
#include "UtilityMacros.h"

template <typename _T>
class CSwapQueue
{
private:
    std::deque<_T> m_writeQueue;
    std::deque<_T> m_readQueue;

    SRWLOCK       m_lock;

public:
    CSwapQueue()
    {
        InitializeSRWLock(&m_lock);
    }
    ~CSwapQueue() = default;

    DELETE_COPY_MOVE(CSwapQueue);

public:
    void Push(_T _val)
    {
        CLockGuard lockGuard(m_lock);
        m_writeQueue.push_back(std::move(_val));
    }
    _T Pop() // 단일 소비자 Thread에서만 호출
    {
        if (m_readQueue.empty())
        {
            return _T{};
        }

        _T val = std::move(m_readQueue.front());
        m_readQueue.pop_front();

        return val;
    }
    void Swap()
    {
        CLockGuard lockGuard(m_lock);
        std::swap(m_writeQueue, m_readQueue);
    }
    void Clear()
    {
        CLockGuard lockGuard(m_lock);
        m_writeQueue.clear();
        m_readQueue.clear();
    }
    uint64_t GetSize()
    {
        CLockGuard lockGuard(m_lock, eLockMode::Shared);
        return m_writeQueue.size() + m_readQueue.size();
    }
};

template <typename _T>
using CMPSCQueue = CSwapQueue<_T>;
