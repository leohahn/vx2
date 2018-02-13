#include "skybox.hpp"
#include "glad/glad.h"
#include "resource_manager.hpp"
#include "lt_utils.hpp"
#include "unit_cube_data.hpp"
#include "renderer.hpp"
#include "application.hpp"

lt_global_variable lt::Logger logger("skybox");

Skybox::Skybox(const char *texture_file, const char *shader_file, const ResourceManager &manager)
    : quad(Mesh())
    , shader(manager.get_shader(shader_file))
    , cubemap(manager.get_texture(texture_file))
{
    if (!shader)
        logger.error("Failed getting shader ", shader_file);

    if (!cubemap)
        logger.error("Failed getting cubemap ", texture_file);
}

Mesh
Skybox::create_mesh()
{
    logger.log("Creating mesh.");

    Mesh mesh;
    mesh.vertices = std::vector<Vec3f>(LT_Count(UNIT_CUBE_VERTICES));

    for (usize i = 0; i < mesh.vertices.size(); i++)
        mesh.vertices[i] = UNIT_CUBE_VERTICES[i];

    for (usize i = 0; i < LT_Count(UNIT_CUBE_INDICES); i+=3)
    {
        Face face = {};
        face.val[0] = UNIT_CUBE_INDICES[i+2];
        face.val[1] = UNIT_CUBE_INDICES[i+1];
        face.val[2] = UNIT_CUBE_INDICES[i];
        mesh.faces.push_back(face);
    }

    Submesh sm = {};
    sm.start_index = 0;
    sm.num_indices = mesh.num_indices();
    sm.textures.push_back(TextureInfo(cubemap->id, "texture_cubemap", GL_TEXTURE_CUBE_MAP));
    mesh.submeshes.push_back(sm);

    // render_setup_mesh_buffers_p(&mesh);
    {
        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vbo);
        glGenBuffers(1, &mesh.ebo);

        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vec3f), &mesh.vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.faces.size() * sizeof(Face), &mesh.faces[0], GL_STATIC_DRAW);

        // vertex positions
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    dump_opengl_errors("create_mesh", __FILE__);
    return mesh;
}

bool
Skybox::load()
{
    if (cubemap->load())
    {
        if (quad.is_invalid()) quad = create_mesh();
        return true;
    }

    return false;
}
