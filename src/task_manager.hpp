#ifndef __TASK_MANAGER_HPP__
#define __TASK_MANAGER_HPP__

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include "lt_core.hpp"
#include "task.hpp"

struct TaskManager
{
    void add_to_queue(std::unique_ptr<Task> task);

    TaskManager();

    std::unique_ptr<Task> get_next_completed();

    void stop();
    void join_thread() { m_task_thread.join(); }

private:
    std::atomic<bool> m_running;

    std::queue<std::unique_ptr<Task>> m_task_queue;
    std::mutex m_task_queue_mutex;

    std::vector<std::unique_ptr<Task>> m_finished_tasks;
    std::thread m_task_thread;

    void run();
};

#endif // __TASK_MANAGER_HPP__
