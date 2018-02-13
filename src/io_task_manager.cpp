#include "io_task_manager.hpp"
#include "lt_utils.hpp"
#include <chrono>

lt_global_variable lt::Logger logger("io_task_manager");

IOTaskManager::IOTaskManager()
    : m_running(false)
    , m_task_thread(&IOTaskManager::run, this)
{
}

void
IOTaskManager::run()
{
    logger.log("Started task manager in thread ", std::this_thread::get_id());
    m_running = true;
    while (m_running)
    {
        IOTask *task = nullptr;

        m_task_queue_mutex.lock();
        if (m_task_queue.size() > 0)
        {
            task = m_task_queue.front();
            m_task_queue.pop();
            logger.log("Running task.");
        }
        m_task_queue_mutex.unlock();

        if (task) task->run();

        // Sleep for a little bit to avoid overusing the CPU.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void
IOTaskManager::add_to_queue(IOTask *task)
{
    m_task_queue_mutex.lock();
    m_task_queue.push(task);
    m_task_queue_mutex.unlock();
}

void
IOTaskManager::stop()
{
    logger.log("Stopping task manager thread.");
    m_running = false;
}
