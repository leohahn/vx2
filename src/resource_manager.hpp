#ifndef __RESOURCE_MANAGER_HPP__
#define __RESOURCE_MANAGER_HPP__

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "glad/glad.h"
#include "lt_core.hpp"
#include "shader.hpp"

struct AsciiFontAtlas;

struct IOTaskManager;
struct LoadImagesTask;

enum TextureFormat
{
    TextureFormat_RGB = GL_RGB8,
    TextureFormat_RGBA = GL_RGBA,
    TextureFormat_SRGB = GL_SRGB8,
    TextureFormat_SRGBA = GL_SRGB_ALPHA,
};

enum PixelFormat
{
    PixelFormat_RGB = GL_RGB,
    PixelFormat_RGBA = GL_RGBA,
};

enum TextureType
{
    TextureType_Unknown,
    TextureType_2D,
    TextureType_Cubemap,
};

struct Texture
{
    TextureType type;
    TextureFormat texture_format;
    PixelFormat pixel_format;

    std::vector<std::string> filepaths;
    u32 id;

    Texture(TextureType type, TextureFormat tf, PixelFormat pf,
            const std::vector<std::string> &filepaths, IOTaskManager *manager);

    inline bool is_loaded() const { return m_is_loaded; }
    bool load();

private:
    bool m_is_loaded;
    IOTaskManager *m_io_task_manager;
    std::unique_ptr<LoadImagesTask> m_task;
};

struct ResourceManager
{
    ResourceManager(IOTaskManager *io_task_manager);

    bool load_from_shader_file(const std::string &filename);
    bool load_from_texture_file(const std::string &filename);
    bool load_from_font_file(const std::string &filename);

    Shader *get_shader(const std::string &filename) const;
    Texture *get_texture(const std::string &filename) const;
    AsciiFontAtlas *get_font(const std::string &filename) const;
    void free_all_resources();

private:
    IOTaskManager *m_io_task_manager;
    std::string m_shaders_path;
    std::string m_textures_path;
    std::string m_fonts_path;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<AsciiFontAtlas>> m_fonts;
};

#endif // __RESOURCE_MANAGER_HPP__
