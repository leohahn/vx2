#include "io_task.hpp"
#include "stb_image.h"
#include "lt_utils.hpp"

lt_global_variable lt::Logger logger("io_task");

lt_internal LoadImagesTask::LoadedImagePtr
load_image(const std::string &filepath)
{
    logger.log("Loading image ", filepath);

    i32 width, height, num_channels;
    uchar *image_data = stbi_load(filepath.c_str(), &width, &height, &num_channels, 0);
    auto loaded_image = std::make_unique<LoadedImage>(width, height, num_channels, image_data);

    return loaded_image;
}

LoadImagesTask::LoadImagesTask(const std::vector<std::string> &filepaths)
    : filepaths(filepaths)
{}

LoadImagesTask::LoadImagesTask(std::string *filepaths, i32 num_files)
    : filepaths(filepaths, filepaths+num_files)
{}

LoadImagesTask::~LoadImagesTask()
{
}

void
LoadImagesTask::run()
{
    m_status = TaskStatus_Processing;

    for (const auto &filepath : filepaths)
    {
        LoadedImagePtr loaded_image = load_image(filepath);
        m_loaded_images.push_back(std::move(loaded_image));
    }

    m_status = TaskStatus_Complete;
}
