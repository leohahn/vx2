#include "resource_manager.hpp"
#include "lt_fs.hpp"
#include "lt_utils.hpp"

lt_global_variable lt::Logger logger("resource_manager");

ResourceManager::ResourceManager()
{
    m_resources_path = ltfs::absolute_path("../resources");
}

bool
ResourceManager::load_from_file(const std::string &filename, ResourceType type)
{
    // NOTE: Currently just create the resource without reading the file.
    // this is probably changing in the future.
    LT_Assert(type == ResourceType_Shader);

    std::string filepath = ltfs::join(m_resources_path, filename);

    if (type == ResourceType_Shader)
    {
        auto new_shader = std::make_unique<Shader>(filepath);

        LT_Assert(ltfs::file_exists(filepath));
        LT_Assert(m_shaders.find(filename) == m_shaders.end());

        m_shaders.insert(std::make_pair(filename, std::move(new_shader)));
    }
    else LT_Assert(false);

    return true;
}

Shader *
ResourceManager::get_shader(const std::string &filename) const
{
    LT_Assert(m_shaders.find(filename) != m_shaders.end());
    return m_shaders.at(filename).get();
}
