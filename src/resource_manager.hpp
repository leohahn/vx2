#ifndef __RESOURCE_MANAGER_HPP__
#define __RESOURCE_MANAGER_HPP__

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "lt_core.hpp"
#include "shader.hpp"
#include "texture.hpp"

struct AsciiFontAtlas;
struct IOTaskManager;
struct LoadImagesTask;

struct ResourceManager
{
    ResourceManager(IOTaskManager *io_task_manager);

    bool load_from_shader_file(const std::string &filename);
    bool load_from_texture_file(const std::string &filename);
    bool load_from_font_file(const std::string &filename);

    Shader *get_shader(const std::string &filename) const;

    template<typename T> T *
    get_texture(const std::string &filename) const
    {
        if (m_textures.find(filename) != m_textures.end())
            return dynamic_cast<T*>(m_textures.at(filename).get());
        else
            return nullptr;
    }

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
