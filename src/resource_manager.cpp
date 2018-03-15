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
#include "texture.hpp"

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

    if (!texture_file.is_file_correct)
    {
        logger.error("Texture file ", filename, " is incorrect.");
        return false;
    }

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

    std::vector<std::string> filepaths;
    TextureFormat texture_format = TextureFormat_RGB;
    PixelFormat pixel_format = PixelFormat_RGB;

    const auto texture_format_entry = texture_file.cast_get<ResourceFile::StringVal>("texture_format");
    const auto pixel_format_entry = texture_file.cast_get<ResourceFile::StringVal>("pixel_format");
    const auto texture_type_entry = texture_file.cast_get<ResourceFile::StringVal>("texture_type");

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
        const auto face_x_pos_entry = texture_file.cast_get<ResourceFile::StringVal>("face_x_pos");
        const auto face_x_neg_entry = texture_file.cast_get<ResourceFile::StringVal>("face_x_neg");
        const auto face_y_pos_entry = texture_file.cast_get<ResourceFile::StringVal>("face_y_pos");
        const auto face_y_neg_entry = texture_file.cast_get<ResourceFile::StringVal>("face_y_neg");
        const auto face_z_pos_entry = texture_file.cast_get<ResourceFile::StringVal>("face_z_pos");
        const auto face_z_neg_entry = texture_file.cast_get<ResourceFile::StringVal>("face_z_neg");

        std::string filepaths[] = {
            ltfs::join(m_textures_path, face_x_pos_entry->str),
            ltfs::join(m_textures_path, face_x_neg_entry->str),
            ltfs::join(m_textures_path, face_y_pos_entry->str),
            ltfs::join(m_textures_path, face_y_neg_entry->str),
            ltfs::join(m_textures_path, face_z_pos_entry->str),
            ltfs::join(m_textures_path, face_z_neg_entry->str)
        };
        LT_Assert(LT_Count(filepaths) == 6);

        auto new_texture = std::make_unique<TextureCubemap>(texture_format, pixel_format,
                                                            filepaths, m_io_task_manager);

        m_textures[filename] = std::move(new_texture);
    }
    else if (texture_type_entry->str == "atlas")
    {
        const auto num_layers_entry = texture_file.cast_get<ResourceFile::IntVal>("num_layers");
        const auto layer_width_entry = texture_file.cast_get<ResourceFile::IntVal>("layer_width");
        const auto layer_height_entry = texture_file.cast_get<ResourceFile::IntVal>("layer_height");
        const auto location_entry = texture_file.cast_get<ResourceFile::StringVal>("texture_location");
        const auto location = ltfs::join(m_textures_path, location_entry->str);

        auto new_texture = std::make_unique<TextureAtlas>(texture_format, pixel_format, location,
                                                          num_layers_entry->number, layer_width_entry->number,
                                                          layer_height_entry->number, m_io_task_manager);
        m_textures[filename] = std::move(new_texture);
    }
    else LT_Assert(false);

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

    if (shader_file.has("textures"))
    {
        auto array_val = shader_file.cast_get<ResourceFile::ArrayVal>("textures");

        new_shader->load();
        for (const auto &v : array_val->vals)
        {
            // FIXME: Assume for the moment that all values inside the array are strings.
            auto string_val = dynamic_cast<ResourceFile::StringVal*>(v.get());
            new_shader->add_texture(string_val->str.c_str());
        }
    }

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
