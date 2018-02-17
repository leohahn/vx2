#ifndef __SKYBOX_HPP__
#define __SKYBOX_HPP__

#include "lt_core.hpp"
#include "mesh.hpp"

struct Shader;
struct ResourceManager;
struct TextureCubemap;

struct Skybox
{
    //
    // TODO: Figure out a way to free the resources allocated by the quad.
    //

    Mesh quad;
    Shader *shader;
    TextureCubemap *cubemap;

    bool load();

    Skybox();
    Skybox(const char *texture_filepath, const char *shader_filepath, const ResourceManager &manager);

private:
    Mesh create_mesh();
};

#endif // __SKYBOX_HPP__
