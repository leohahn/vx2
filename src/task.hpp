#ifndef __TASK_HPP__
#define __TASK_HPP__

#include <atomic>
#include <memory>
#include "lt_fs.hpp"

struct IOTaskManager;

enum IOTaskStatus
{
    TaskStatus_Complete,
    TaskStatus_Failed,
};

struct IOTask
{
    friend struct IOTaskManager;
    FileContents *data() { return m_data; }

protected:
    FileContents *m_data;
    std::atomic<IOTaskStatus> m_status;

    virtual void operator()() = 0;

private:
    void set_status(IOTaskStatus status) { m_status = status; }
};

struct LoadTextFileTask : IOTask
{
    LoadTextFileTask(const char *path)
        : filepath(path)
    {}

    void
    operator()() override
    {
        m_data = file_read_contents(filepath);
        LT_Assert(m_data);
        m_status = TaskStatus_Complete;
    }

    const char *filepath;
};

#endif // __TASK_HPP__
