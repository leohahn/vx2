#include "task_manager.hpp"
#include "lt_utils.hpp"
#include <chrono>

lt_global_variable lt::Logger logger("task_manager");

TaskManager::TaskManager()
    : m_running(false)
    , m_task_thread(&TaskManager::run, this)
{
}

void
TaskManager::run()
{
    logger.log("Started task manager in thread ", std::this_thread::get_id());
    while (m_running)
    {
        while (m_task_queue.size() > 0)
        {
            std::unique_ptr<Task> front_task = std::move(m_task_queue.front());
            m_task_queue.pop();
            front_task->process();
            m_finished_tasks.push_back(std::move(front_task));
        }

        // Sleep for a little bit to avoid overusing the CPU.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::unique_ptr<Task>
TaskManager::get_next_completed()
{

}

void
TaskManager::add_to_queue(std::unique_ptr<Task> task)
{
    m_task_queue_mutex.lock();
    m_task_queue.push(std::move(task));
    m_task_queue_mutex.unlock();
}

void
TaskManager::stop()
{
    logger.log("Stopping task manager thread.");
    m_running = false;
}
