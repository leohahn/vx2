#include "texture.hpp"
#include "lt_utils.hpp"
#include "application.hpp"
#include "io_task_manager.hpp"

lt_global_variable lt::Logger logger("texture");

// -----------------------------------------------------------------------------
// General Functions
// -----------------------------------------------------------------------------

lt_internal u32
create_texture_id(TextureType type, TextureFormat texture_format, PixelFormat pixel_format,
                  const std::vector<std::unique_ptr<LoadedImage>> &loaded_images)
{
    u32 texture_id = 0;

    switch (type)
    {
    case TextureType_2D: {
        LT_Assert(loaded_images.size() == 1);
        LT_Assert(false);
        break;
    }
    case TextureType_Cubemap: {
        LT_Assert(loaded_images.size() == 6);

        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

        for (u32 i = 0; i < loaded_images.size(); i++)
        {
            const auto &li = loaded_images[i];

            const i32 mipmap_level = 0;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipmap_level, texture_format, li->width, li->height,
                         0, pixel_format, GL_UNSIGNED_BYTE, li->data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        dump_opengl_errors("After cubemap creation", __FILE__);
        break;
    }
    case TextureType_Unknown:
    default: LT_Assert(false);
    }

    LT_Assert(texture_id != 0);

    return texture_id;
}

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
        id = create_texture_id(type, texture_format, pixel_format, m_task->view_loaded_images());
        logger.log("id ", id);
        m_task.reset(); // destroy task object and free its resources.
    }

    return m_is_loaded;
}

// -----------------------------------------------------------------------------
// Texture Atlas
// -----------------------------------------------------------------------------

TextureAtlas::TextureAtlas(TextureFormat tf, PixelFormat pf, const std::string &filepath,
                           i32 num_tile_rows, i32 num_tile_cols, i32 tile_width, i32 tile_height,
                           IOTaskManager *manager)
    : Texture(TextureType_2D, tf, pf)
    , filepath(filepath)
    , m_task(nullptr)
    , m_io_task_manager(manager)
    , m_num_tile_rows(num_tile_rows)
    , m_num_tile_cols(num_tile_cols)
    , m_tile_width(tile_width)
    , m_tile_height(tile_height)
{
    logger.log("Creating texture atlas ", filepath);
}

bool
TextureAtlas::load()
{
    logger.log("Calling TEXTURE ATLAS LOAD.");
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
        id = create_texture_id(type, texture_format, pixel_format, m_task->view_loaded_images());
        logger.log(filepath, ": id ", id);
        m_task.reset(); // destroy task object and free its resources.
    }

    return m_is_loaded;
}
