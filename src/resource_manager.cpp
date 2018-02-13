#include "resource_manager.hpp"
#include "lt_fs.hpp"
#include "lt_utils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "io_task.hpp"
#include "io_task_manager.hpp"
#include "application.hpp"

lt_global_variable lt::Logger logger("resource_manager");

lt_global_variable const std::vector<std::string> entry_keys = {
    "texture_type",
    "texture_format",
    "pixel_format",
    "face_x_pos",
    "face_x_neg",
    "face_y_pos",
    "face_y_neg",
    "face_z_pos",
    "face_z_neg",
};

struct TextureFile
{
    TextureType type;
    TextureFormat texture_format;
    PixelFormat pixel_format;

    std::unordered_map<std::string, std::string> entries;
    std::string filepath;
    bool is_file_correct = true;

    TextureFile(std::string filepath)
        : type(TextureType_Unknown)
        , filepath(filepath)
    {
        std::ifstream infile(filepath);
        std::string line;
        while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            std::string key, val;
            char eq;

            bool success_read = (bool)(iss >> key >> eq >> val);

            // Skip comments
            if (key.size() > 0 && key[0] == '#')
                continue;

            if (!success_read && eq == '=')
            {
                logger.error("Loading file ", filepath, ": incorrect syntax");
                is_file_correct = false;
                break;
            }

            if (std::find(entry_keys.begin(), entry_keys.end(), key) == entry_keys.end())
            {
                logger.error("On file ", filepath);
                logger.error("Invalid key ", key);
                is_file_correct = false;
                break;
            }

            if (key == "texture_type")
            {
                if (val == "cubemap") type = TextureType_Cubemap;
                if (val == "2D") type = TextureType_2D;
            }

            if (key == "texture_format")
            {
                if (val == "rgb") texture_format = TextureFormat_RGB;
                else if (val == "rgba") texture_format = TextureFormat_RGBA;
                else if (val == "srgb") texture_format = TextureFormat_SRGB;
                else if (val == "srgba") texture_format = TextureFormat_SRGBA;
                else logger.error("Wrong texture format specified: ", val);
            }

            if (key == "pixel_format")
            {
                if (val == "rgb") pixel_format = PixelFormat_RGB;
                else if (val == "rgba") pixel_format = PixelFormat_RGBA;
                else logger.error("Wrong pixel format specified: ", val);
            }

            entries.insert(std::make_pair(key, val));
        }
    }
};

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

    logger.log("Textures path: ", m_textures_path);
    logger.log("Shaders path: ", m_shaders_path);
}

bool
ResourceManager::load_from_texture_file(const std::string &filename)
{
    if (m_textures.find(filename) != m_textures.end())
    {
        logger.error("File ", filename, " was already loaded.");
        return false;
    }

    std::string filepath = ltfs::join(m_textures_path, filename);
    TextureFile texture_file(filepath);

    if (!texture_file.is_file_correct)
    {
        logger.error("Texture file ", filename, " is incorrect.");
        return false;
    }

    std::vector<std::string> filepaths;
    if (texture_file.type == TextureType_Cubemap)
    {
        filepaths.push_back(ltfs::join(m_textures_path, texture_file.entries.at("face_x_pos")));
        filepaths.push_back(ltfs::join(m_textures_path, texture_file.entries.at("face_x_neg")));
        filepaths.push_back(ltfs::join(m_textures_path, texture_file.entries.at("face_y_pos")));
        filepaths.push_back(ltfs::join(m_textures_path, texture_file.entries.at("face_y_neg")));
        filepaths.push_back(ltfs::join(m_textures_path, texture_file.entries.at("face_z_pos")));
        filepaths.push_back(ltfs::join(m_textures_path, texture_file.entries.at("face_z_neg")));
    }
    else
    {
        LT_Assert(false);
    }

    m_textures[filename] = std::make_unique<Texture>(texture_file.type,
                                                     texture_file.texture_format,
                                                     texture_file.pixel_format,
                                                     filepaths,
                                                     m_io_task_manager);
    return true;
}

bool
ResourceManager::load_from_shader_file(const std::string &filename)
{
    // NOTE: Currently just create the resource without reading the file.
    // this is probably changing in the future.

    std::string filepath = ltfs::join(m_shaders_path, filename);

    if (!ltfs::file_exists(filepath))
    {
        logger.error("File ", filepath, " does not exist");
        return false;
    }

    auto new_shader = std::make_unique<Shader>(filepath);

    LT_Assert(m_shaders.find(filename) == m_shaders.end());

    m_shaders.insert(std::make_pair(filename, std::move(new_shader)));

    return true;
}

Shader *
ResourceManager::get_shader(const std::string &filename) const
{
    if (m_shaders.find(filename) != m_shaders.end())
        return m_shaders.at(filename).get();
    else
        return nullptr;
}

Texture *
ResourceManager::get_texture(const std::string &filename) const
{
    if (m_textures.find(filename) != m_textures.end())
        return m_textures.at(filename).get();
    else
        return nullptr;
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
