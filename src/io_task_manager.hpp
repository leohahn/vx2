#ifndef __IO_TASK_MANAGER_HPP__
#define __IO_TASK_MANAGER_HPP__

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <condition_variable>
#include "lt_core.hpp"
#include "io_task.hpp"

struct IOTaskManager
{
    void add_to_queue(IOTask *task);

    IOTaskManager();
    ~IOTaskManager();

private:
    std::atomic<bool> m_thread_running;
    std::condition_variable m_task_added;
    bool m_wake_up_thread;

    std::queue<IOTask*> m_task_queue;
    std::mutex m_task_queue_mutex;

    std::thread m_task_thread;

    void run();
    void stop();
};

#endif // __IO_TASK_MANAGER_HPP__
