#include "resource_manager.hpp"
#include "lt_fs.hpp"
#include "lt_utils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "io_task.hpp"
#include "io_task_manager.hpp"
#include "application.hpp"
#include "font.hpp"
#include "resource_file.hpp"

lt_global_variable lt::Logger logger("resource_manager");

ResourceManager::ResourceManager(IOTaskManager *io_task_manager)
    : m_io_task_manager(io_task_manager)
{
    bool error;
    m_shaders_path = ltfs::absolute_path("../resources/shaders", &error);
    if (error)
        logger.error("Cannot resolve relative path '../resources/shaders'");

    m_textures_path = ltfs::absolute_path("../resources/textures", &error);
    if (error)
        logger.error("Cannot resolve relative path '../resources/textures'");

    m_fonts_path = ltfs::absolute_path("../resources/fonts", &error);
    if (error)
        logger.error("Cannot resolve relative path '../resources/fonts'");

    logger.log("Textures path: ", m_textures_path);
    logger.log("Shaders path: ", m_shaders_path);
    logger.log("Fonts path: ", m_fonts_path);
}

bool
ResourceManager::load_from_texture_file(const std::string &filename)
{
    if (m_textures.find(filename) != m_textures.end())
    {
        logger.error("File ", filename, " was already loaded.");
        return false;
    }

    logger.log("Loading from texture ", filename);

    std::string filepath = ltfs::join(m_textures_path, filename);
    ResourceFile texture_file(filepath);
    texture_file.parse();

    if (!texture_file.is_file_correct)
    {
        logger.error("Texture file ", filename, " is incorrect.");
        return false;
    }

    std::vector<std::string> filepaths;
    TextureType type = TextureType_Unknown;
    TextureFormat texture_format = TextureFormat_RGB;
    PixelFormat pixel_format = PixelFormat_RGB;

    if (!texture_file.has("texture_format"))
    {
        logger.error("File needs a texture_format entry.");
        return false;
    }

    if (!texture_file.has("pixel_format"))
    {
        logger.error("File needs a pixel_format entry.");
        return false;
    }

    if (!texture_file.has("texture_type"))
    {
        logger.error("File needs a pixel_format entry.");
        return false;
    }

    auto texture_format_entry = texture_file.cast_get<ResourceFile::StringVal>("texture_format");
    auto pixel_format_entry = texture_file.cast_get<ResourceFile::StringVal>("pixel_format");
    auto texture_type_entry = texture_file.cast_get<ResourceFile::StringVal>("texture_type");

    if (texture_format_entry->str == "rgb")
        texture_format = TextureFormat_RGB;
    else if (texture_format_entry->str == "rgba")
        texture_format = TextureFormat_RGBA;
    else if (texture_format_entry->str == "srgba")
        texture_format = TextureFormat_SRGBA;
    else if (texture_format_entry->str == "srgb")
        texture_format = TextureFormat_SRGB;

    if (pixel_format_entry->str == "rgb")
        pixel_format = PixelFormat_RGB;
    else if (pixel_format_entry->str == "rgba")
        pixel_format = PixelFormat_RGBA;

    if (texture_type_entry->str == "cubemap")
    {
        auto face_x_pos_entry = texture_file.cast_get<ResourceFile::StringVal>("face_x_pos");
        auto face_x_neg_entry = texture_file.cast_get<ResourceFile::StringVal>("face_x_neg");
        auto face_y_pos_entry = texture_file.cast_get<ResourceFile::StringVal>("face_y_pos");
        auto face_y_neg_entry = texture_file.cast_get<ResourceFile::StringVal>("face_y_neg");
        auto face_z_pos_entry = texture_file.cast_get<ResourceFile::StringVal>("face_z_pos");
        auto face_z_neg_entry = texture_file.cast_get<ResourceFile::StringVal>("face_z_neg");

        type = TextureType_Cubemap;
        filepaths.push_back(ltfs::join(m_textures_path, face_x_pos_entry->str));
        filepaths.push_back(ltfs::join(m_textures_path, face_x_neg_entry->str));
        filepaths.push_back(ltfs::join(m_textures_path, face_y_pos_entry->str));
        filepaths.push_back(ltfs::join(m_textures_path, face_y_neg_entry->str));
        filepaths.push_back(ltfs::join(m_textures_path, face_z_pos_entry->str));
        filepaths.push_back(ltfs::join(m_textures_path, face_z_neg_entry->str));
    }
    else
    {
        LT_Assert(false);
    }

    m_textures[filename] = std::make_unique<Texture>(type, texture_format, pixel_format,
                                                     filepaths, m_io_task_manager);
    return true;
}

bool
ResourceManager::load_from_shader_file(const std::string &filename)
{
    if (m_shaders.find(filename) != m_shaders.end())
    {
        logger.error("File ", filename, " was already loaded.");
        return false;
    }

    std::string filepath = ltfs::join(m_shaders_path, filename);
    if (!ltfs::file_exists(filepath))
    {
        logger.error("File ", filepath, " does not exist");
        return false;
    }

    ResourceFile shader_file(filepath);
    shader_file.parse();
    if (!shader_file.is_file_correct)
    {
        logger.error("File ", filename, " is incorrect.");
        return false;
    }

    std::string shader_filepath;
    if (shader_file.has("shader_source"))
    {
        auto *val = shader_file.cast_get<ResourceFile::StringVal>("shader_source");
        shader_filepath = ltfs::join(m_shaders_path, val->str);
    }
    else
    {
        logger.log("Shader file ", filename, " needs a 'shader_source' key");
        return false;
    }

    logger.log("Creating shader with source ", shader_filepath);

    auto new_shader = std::make_unique<Shader>(shader_filepath);
    m_shaders.insert(std::make_pair(filename, std::move(new_shader)));

    return true;
}

bool
ResourceManager::load_from_font_file(const std::string &filename)
{
    std::string filepath = ltfs::join(m_fonts_path, filename);

    if (!ltfs::file_exists(filepath))
    {
        logger.error("File ", filepath, " does not exist");
        return false;
    }

    bool error;
    std::string abs_filepath = ltfs::absolute_path(filepath, &error);
    LT_Assert(!error);

    auto new_font = std::make_unique<AsciiFontAtlas>(abs_filepath, 512, 512);

    LT_Assert(m_shaders.find(filename) == m_shaders.end());

    m_fonts.insert(std::make_pair(filename, std::move(new_font)));

    return true;
}

Shader *
ResourceManager::get_shader(const std::string &filename) const
{
    if (m_shaders.find(filename) != m_shaders.end())
        return m_shaders.at(filename).get();
    else
    {
        logger.error("Could not get shader ", filename);
        return nullptr;
    }
}

Texture *
ResourceManager::get_texture(const std::string &filename) const
{
    if (m_textures.find(filename) != m_textures.end())
        return m_textures.at(filename).get();
    else
    {
        logger.error("Could not get texture ", filename);
        return nullptr;
    }
}

AsciiFontAtlas *
ResourceManager::get_font(const std::string &filename) const
{
    if (m_fonts.find(filename) != m_fonts.end())
        return m_fonts.at(filename).get();
    else
    {
        logger.error("Could not get font ", filename);
        return nullptr;
    }
}

Texture::Texture(TextureType type, TextureFormat tf, PixelFormat pf,
        const std::vector<std::string> &filepaths, IOTaskManager *manager)
    : type(type)
    , texture_format(tf)
    , pixel_format(pf)
    , filepaths(filepaths)
    , id(0)
    , m_is_loaded(false)
    , m_io_task_manager(manager)
    , m_task(nullptr)
{
    logger.log("Creating texture file");
}

lt_internal u32
create_texture_id(TextureType type, TextureFormat texture_format, PixelFormat pixel_format,
                  const std::vector<std::unique_ptr<LoadedImage>> &loaded_images)
{
    u32 texture_id = 0;

    switch (type)
    {
    case TextureType_Unknown:
    case TextureType_2D:
        LT_Assert(false);
        break;
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
    default: LT_Assert(false);
    }

    LT_Assert(texture_id != 0);

    return texture_id;
}

bool
Texture::load()
{
    if (!m_task && id == 0)
    {
        logger.log("Adding task to queue.");
        m_task = std::make_unique<LoadImagesTask>(filepaths);
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
