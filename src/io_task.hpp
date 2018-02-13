#ifndef __TASK_HPP__
#define __TASK_HPP__

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include "glad/glad.h"
#include "lt_fs.hpp"
#include "stb_image.h"

struct IOTaskManager;

struct LoadedImage
{
    i32 width;
    i32 height;
    i32 num_channels;
    uchar *data;

    LoadedImage(i32 width, i32 height, i32 num_channels, uchar *data)
        : width(width)
        , height(height)
        , num_channels(num_channels)
        , data(data)
    {}

    ~LoadedImage()
    {
        stbi_image_free(data);
    }
};

enum IOTaskStatus
{
    // An assumption is made that whenever the status is Complete its resources are thread safe.
    // However when the status is on Processing, another thread may be working on the task's resources,
    // therefore accessing them is considered unsafe. This avoid the need for a mutex.
    // TODO: Evalueate if this is a good idea.

    TaskStatus_Processing,
    TaskStatus_Complete,
};

struct IOTask
{
    friend struct IOTaskManager;

    virtual ~IOTask() {};

    IOTaskStatus status() const { return m_status; }

protected:
    std::atomic<IOTaskStatus> m_status;
    virtual void run() = 0;
};

struct LoadImagesTask : IOTask
{
    using LoadedImagePtr = std::unique_ptr<LoadedImage>;

    LoadImagesTask(const std::vector<std::string> &filepaths);
    ~LoadImagesTask();

    void run() override;

    inline const std::vector<LoadedImagePtr> &view_loaded_images() const
    {
        return m_loaded_images;
    }

    std::vector<std::string> filepaths;

private:
    std::vector<LoadedImagePtr> m_loaded_images;
};

#endif // __TASK_HPP__
