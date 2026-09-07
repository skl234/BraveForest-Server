#pragma once
#include <WinSock2.h>
#include <deque>
#include <memory>
#include "LockGuard.h"

template <typename _T>
class CThreadSafeQueue
{
private:
    std::deque<_T> m_queue;
    SRWLOCK        m_lock;

public:
    CThreadSafeQueue()
    {
        InitializeSRWLock(&m_lock);
    }
    ~CThreadSafeQueue() = default;

public:
    void Push(_T _val)
    {
        CLockGuard guard(m_lock);
        m_queue.push_back(std::move(_val));
    }
    bool Pop(_T& _out)
    {
        CLockGuard guard(m_lock);
        if (m_queue.empty()) return false;
        _out = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }
    void Clear()
    {
        CLockGuard guard(m_lock);
        m_queue.clear();
    }
    bool IsEmpty()
    {
        CLockGuard guard(m_lock, eLockMode::Shared);
        return m_queue.empty();
    }
    uint64_t GetSize()
    {
        CLockGuard guard(m_lock, eLockMode::Shared);
        return m_queue.size();
    }
};

template <typename _T>
using CMPMCQueue = CThreadSafeQueue<_T>;
