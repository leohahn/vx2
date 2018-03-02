#include "renderer.hpp"
#include <mutex>
#include "glad/glad.h"
#include "world.hpp"
#include "application.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "skybox.hpp"
#include "mesh.hpp"
#include "font.hpp"
#include "vertex.hpp"

lt_global_variable lt::Logger logger("renderer");

lt_internal void
render_mesh(const Mesh &mesh, Shader *shader)
{
    for (usize i = 0; i < mesh.submeshes.size(); i++)
    {
				Submesh sm = mesh.submeshes[i];

				for (usize t = 0; t < sm.textures.size(); t++)
				{
            const std::string &name = sm.textures[t].name;
            const u32 texture_unit = shader->texture_unit(name);
            glActiveTexture(GL_TEXTURE0 + texture_unit);
            glBindTexture(sm.textures[t].type, sm.textures[t].id);
				}

        LT_Assert(mesh.vao != 0);
        glBindVertexArray(mesh.vao);
				glDrawElements(GL_TRIANGLES, sm.num_indices, GL_UNSIGNED_INT, (const void*)sm.start_index);
        glBindVertexArray(0);
    }
}

// void
// render_final_quad(const Application &app, const Camera &camera, Shader *shader)
// {
//     render_mesh(app.render_quad, shader);
// }

void
render_world(World &world) // TODO: maybe change this to render_landscape??
{
    // Assuming that every chunk uses the same shader program.
    for (i32 i = 0; i < Landscape::NUM_CHUNKS; i++)
    {
        std::lock_guard<std::mutex> lock(world.landscape->vao_array.mutex);
        auto entry = world.landscape->vao_array.vaos[i];
        if (entry.is_used && entry.num_vertices_used > 0)
        {
            LT_Assert(entry.vao != 0); // The vao should already be created.
            LT_Assert(entry.vbo != 0); // The vbo should already be created.

            glBindVertexArray(entry.vao);
            glDrawArrays(GL_TRIANGLES, 0, entry.num_vertices_used);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

void
render_skybox(const Skybox &skybox)
{
    glDepthFunc(GL_LEQUAL);
    render_mesh(skybox.quad, skybox.shader);
    glDepthFunc(GL_LESS);
}

void
render_setup_mesh_buffers_p(Mesh *m)
{
    glGenVertexArrays(1, &m->vao);
    glGenBuffers(1, &m->vbo);
    glGenBuffers(1, &m->ebo);

    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);

    glBufferData(GL_ARRAY_BUFFER, m->vertices.size() * sizeof(Vec3f), &m->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->faces.size() * sizeof(Face), &m->faces[0], GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void
render_text(AsciiFontAtlas *atlas, const std::string &text, f32 posx, f32 posy, Shader *shader)
{
    // TODO, SPEED: Add a static array here, instead of allocating a new vector each frame.
    std::vector<Vertex_PU> text_buf = atlas->render_text_to_buffer(text, posx, posy);

    glBindVertexArray(atlas->vao);
    glBindBuffer(GL_ARRAY_BUFFER, atlas->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_PU)*text_buf.size(), &text_buf[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, tex_coords));
    glEnableVertexAttribArray(1);

    shader->use();

    glActiveTexture(GL_TEXTURE0 + shader->texture_unit("font_atlas"));
    glBindTexture(GL_TEXTURE_2D, atlas->id);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_TRIANGLES, 0, text_buf.size());

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

void
render_loading_screen(const Application &app, AsciiFontAtlas *atlas, Shader *font_shader)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_text(atlas, "Loading...", app.screen_width/2.3, app.screen_height/2, font_shader);
}
