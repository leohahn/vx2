#ifndef __IO_TASK_MANAGER_HPP__
#define __IO_TASK_MANAGER_HPP__

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include <unordered_map>
#include "lt_core.hpp"
#include "io_task.hpp"

typedef i32 IOTaskHandle;

struct IOTaskManager
{
    void add_to_queue(IOTask *task);

    IOTaskManager();

    void stop();
    void join_thread() { m_task_thread.join(); }

private:
    std::atomic<bool> m_running;

    std::queue<IOTask*> m_task_queue;
    std::mutex m_task_queue_mutex;

    std::thread m_task_thread;

    void run();
};

#endif // __IO_TASK_MANAGER_HPP__
