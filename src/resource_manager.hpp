#ifndef __RESOURCE_MANAGER_HPP__
#define __RESOURCE_MANAGER_HPP__

#include <string>
#include <memory>
#include <unordered_map>
#include "lt_core.hpp"
#include "shader.hpp"

enum ResourceType
{
    ResourceType_Shader,
};

struct ResourceManager
{
    ResourceManager();

    bool load_from_file(const std::string &filename, ResourceType type);
    Shader *get_shader(const std::string &filename) const;
    void free_all_resources();

private:
    std::string m_resources_path;
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
};

#endif // __RESOURCE_MANAGER_HPP__
