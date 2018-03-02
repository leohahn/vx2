#ifndef __SEMAPHORE_HPP__
#define __SEMAPHORE_HPP__

#include <condition_variable>
#include <mutex>

#include "lt_core.hpp"

struct Semaphore
{
    Semaphore() : m_count(0) {}

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count++;
        m_signal.notify_one();
    }

    inline void notify_all(i32 count = 1)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_count += count;
        m_signal.notify_all();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_count == 0)
            m_signal.wait(lock);
        m_count--;
    }

private:
    std::condition_variable m_signal;
    std::mutex m_mutex;
    u32 m_count;
};

#endif // __SEMAPHORE_HPP__
