#include "io_task_manager.hpp"
#include "lt_utils.hpp"
#include <chrono>

lt_global_variable lt::Logger logger("io_task_manager");

IOTaskManager::IOTaskManager()
    : m_thread_running(false)
    , m_wake_up_thread(false)
    , m_task_thread(&IOTaskManager::run, this)
{
}

void
IOTaskManager::run()
{
    logger.log("Started task manager in thread ", std::this_thread::get_id());
    m_thread_running = true;
    while (m_thread_running)
    {
        std::unique_lock<std::mutex> locker(m_task_queue_mutex);

        m_task_added.wait(locker, [&]{ return m_wake_up_thread; });

        while (!m_task_queue.empty())
        {
            m_task_queue.front()->run();
            m_task_queue.pop();
            logger.log("Running task.");
        }

        m_wake_up_thread = false;
    }
}

void
IOTaskManager::add_to_queue(IOTask *task)
{
    std::unique_lock<std::mutex> locker(m_task_queue_mutex);
    m_task_queue.push(task);
    m_task_added.notify_one();
    m_wake_up_thread = true;
}

void
IOTaskManager::stop()
{
    logger.log("Stopping task manager thread.");
    m_thread_running = false;
    // NOTE: A "work" signal is sent to the thread, however there is no work to do.
    // This is only done to wake up the thread so it can check that the variable 'm_thread_running'
    // is false so it can exit.
    m_wake_up_thread = true;
    m_task_added.notify_one();
}
