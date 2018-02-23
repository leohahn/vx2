#include "texture.hpp"
#include "lt_utils.hpp"
#include "application.hpp"
#include "io_task_manager.hpp"

#include "stb_image_write.h"

lt_global_variable lt::Logger logger("texture");

// -----------------------------------------------------------------------------
// Cubemap
// -----------------------------------------------------------------------------

TextureCubemap::TextureCubemap(TextureFormat tf, PixelFormat pf, std::string *paths, IOTaskManager *manager)
    : Texture(TextureType_Cubemap, tf, pf)
    , m_task(nullptr)
    , m_io_task_manager(manager)
{
    for (i32 i = 0; i < NUM_CUBEMAP_FACES; i++)
        filepaths[i] = paths[i];

    logger.log("Creating cubemap");
}

bool
TextureCubemap::load()
{
    if (!m_task && id == 0)
    {
        logger.log("Adding task to queue.");
        m_task = std::make_unique<LoadImagesTask>(filepaths, NUM_CUBEMAP_FACES);
        m_io_task_manager->add_to_queue(m_task.get());
    }

    if (m_task)
        m_is_loaded = (m_task->status() == TaskStatus_Complete);

    if (m_is_loaded && id == 0)
    {
        logger.log("Creating texture");
        {
            const std::vector<std::unique_ptr<LoadedImage>> &loaded_images = m_task->view_loaded_images();
            LT_Assert(loaded_images.size() == 6);

            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_CUBE_MAP, id);

            for (u32 i = 0; i < loaded_images.size(); i++)
            {
                const auto &li = loaded_images[i];

                const i32 mipmap_level = 0;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipmap_level,
                             texture_format, li->width, li->height,
                             0, pixel_format, GL_UNSIGNED_BYTE, li->data);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            dump_opengl_errors("After cubemap creation", __FILE__);
        }
        logger.log("id ", id);
        m_task.reset(); // destroy task object and free its resources.
    }

    return m_is_loaded;
}

// -----------------------------------------------------------------------------
// Texture Atlas
// -----------------------------------------------------------------------------

TextureAtlas::TextureAtlas(TextureFormat tf, PixelFormat pf, const std::string &filepath,
                           i32 num_layers, i32 layer_width, i32 layer_height, IOTaskManager *manager)
    : Texture(TextureType_2D_Array, tf, pf)
    , filepath(filepath)
    , num_layers(num_layers)
    , layer_width(layer_width)
    , layer_height(layer_height)
    , m_task(nullptr)
    , m_io_task_manager(manager)
{
    logger.log("Creating texture atlas ", filepath);
}

bool
TextureAtlas::load()
{
    if (!m_task && id == 0)
    {
        logger.log("Adding task to queue.");
        m_task = std::make_unique<LoadImagesTask>(&filepath, 1);
        m_io_task_manager->add_to_queue(m_task.get());
    }

    if (m_task)
        m_is_loaded = (m_task->status() == TaskStatus_Complete);

    if (m_is_loaded && id == 0)
    {
        logger.log("Creating texture");
        {
            const std::vector<std::unique_ptr<LoadedImage>> &loaded_images = m_task->view_loaded_images();
            LT_Assert(loaded_images.size() == 1);

            width = loaded_images[0]->width;
            height = loaded_images[0]->height;

            logger.log("Texture sized (", width, ", ", height, ")");
            logger.log("Num layers: ", num_layers);

            LT_Assert(width == layer_width);
            LT_Assert(height == num_layers*layer_height);

            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D_ARRAY, id);
            const i32 mipmap_level = 0;

            glTexImage3D(GL_TEXTURE_2D_ARRAY, mipmap_level, texture_format,
                         layer_width, layer_height, num_layers,
                         0, pixel_format, GL_UNSIGNED_BYTE, loaded_images[0]->data);

            // TODO: Should these be parameters?
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

            dump_opengl_errors("After texture 2D array creation", __FILE__);
        }
        logger.log(filepath, ": id ", id);
        m_task.reset(); // destroy task object and free its resources.
    }

    return m_is_loaded;
}
