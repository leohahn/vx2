#include "renderer.hpp"
#include "glad/glad.h"
#include "world.hpp"
#include "application.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "skybox.hpp"
#include "mesh.hpp"

lt_global_variable lt::Logger logger("renderer");


lt_internal inline Vec3f
get_world_coords(const Chunk &chunk, i32 block_xi, i32 block_yi, i32 block_zi)
{
    Vec3f coords;
    coords.x = chunk.origin.x + (block_xi * Chunk::BLOCK_SIZE);
    coords.y = chunk.origin.y + (block_yi * Chunk::BLOCK_SIZE);
    coords.z = chunk.origin.z + (block_zi * Chunk::BLOCK_SIZE);
    return coords;
}

struct Vertex_PUN
{
    Vec3f position;
    Vec2f tex_coords;
    Vec3f normal;
};

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

void
render_final_quad(const Application &app, const Camera &camera, Shader *shader)
{
    render_mesh(app.render_quad, shader);
}

lt_internal void
render_chunk(const World &world, Chunk &chunk)
{
    const Vec3f pos_x_offset(Chunk::BLOCK_SIZE, 0, 0);
    const Vec3f pos_y_offset(0, Chunk::BLOCK_SIZE, 0);
    const Vec3f pos_z_offset(0, 0, Chunk::BLOCK_SIZE);

    constexpr i32 MAX_NUM_VERTICES = Chunk::NUM_BLOCKS * 36;
    lt_local_persist Vertex_PUN chunk_vertices[MAX_NUM_VERTICES];

    // Set the array to zero for each call of the function.
    std::memset(chunk_vertices, 0, sizeof(chunk_vertices));

    i32 num_vertices_used = 0; // number of vertices used in the array.

    for (i32 block_xi = 0; block_xi < Chunk::NUM_BLOCKS_PER_AXIS; block_xi++)
        for (i32 block_yi = 0; block_yi < Chunk::NUM_BLOCKS_PER_AXIS; block_yi++)
            for (i32 block_zi = 0; block_zi < Chunk::NUM_BLOCKS_PER_AXIS; block_zi++)
            {
                const BlockType block_type = chunk.blocks[block_xi][block_yi][block_zi];

                if (block_type == BlockType_Air)
                    continue;

                // Where the block is located in world space.
                const Vec3f block_origin = get_world_coords(chunk, block_xi, block_yi, block_zi);

                // All possible vertices of the block.
                const Vec3f left_bottom_back = block_origin;
                const Vec3f left_bottom_front = left_bottom_back + pos_z_offset;
                const Vec3f left_top_front = left_bottom_front + pos_y_offset;
                const Vec3f left_top_back = left_top_front - pos_z_offset;
                const Vec3f right_bottom_back = left_bottom_back + pos_x_offset;
                const Vec3f right_top_back = right_bottom_back + pos_y_offset;
                const Vec3f right_top_front = right_top_back + pos_z_offset;
                const Vec3f right_bottom_front = right_top_front - pos_y_offset;

                // Left face ------------------------------------------------------
                chunk_vertices[num_vertices_used].position   = left_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                chunk_vertices[num_vertices_used].position   = left_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                chunk_vertices[num_vertices_used].position   = left_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                chunk_vertices[num_vertices_used].position   = left_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                chunk_vertices[num_vertices_used].position   = left_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                chunk_vertices[num_vertices_used].position   = left_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);
                // Right face -----------------------------------------------------
                chunk_vertices[num_vertices_used].position   = right_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);

                chunk_vertices[num_vertices_used].position   = right_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);

                chunk_vertices[num_vertices_used].position   = right_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal     = Vec3f(1, 0, 0);

                chunk_vertices[num_vertices_used].position   = right_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal     = Vec3f(1, 0, 0);

                chunk_vertices[num_vertices_used].position   = right_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);

                chunk_vertices[num_vertices_used].position   = right_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);
                // Top face --------------------------------------------------------
                chunk_vertices[num_vertices_used].position   = left_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                chunk_vertices[num_vertices_used].position   = right_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                chunk_vertices[num_vertices_used].position   = right_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                chunk_vertices[num_vertices_used].position   = right_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                chunk_vertices[num_vertices_used].position   = left_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                chunk_vertices[num_vertices_used].position   = left_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);
                // Bottom face -----------------------------------------------------
                chunk_vertices[num_vertices_used].position   = left_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                chunk_vertices[num_vertices_used].position   = right_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                chunk_vertices[num_vertices_used].position   = right_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                chunk_vertices[num_vertices_used].position   = right_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                chunk_vertices[num_vertices_used].position   = left_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                chunk_vertices[num_vertices_used].position   = left_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);
                // Front face -------------------------------------------------------
                chunk_vertices[num_vertices_used].position   = left_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                chunk_vertices[num_vertices_used].position   = right_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                chunk_vertices[num_vertices_used].position   = right_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                chunk_vertices[num_vertices_used].position   = right_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                chunk_vertices[num_vertices_used].position   = left_top_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                chunk_vertices[num_vertices_used].position   = left_bottom_front;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);
                // Back face --------------------------------------------------------
                chunk_vertices[num_vertices_used].position   = right_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                chunk_vertices[num_vertices_used].position   = left_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                chunk_vertices[num_vertices_used].position   = left_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                chunk_vertices[num_vertices_used].position   = left_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(1, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                chunk_vertices[num_vertices_used].position   = right_top_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 1);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                chunk_vertices[num_vertices_used].position   = right_bottom_back;
                chunk_vertices[num_vertices_used].tex_coords = Vec2f(0, 0);
                chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);
            }

    LT_Assert(num_vertices_used <= MAX_NUM_VERTICES);
    LT_Assert(chunk.vbo != 0); // The vbo should already be created.

    glBindVertexArray(chunk.vao);
    dump_opengl_errors("glBindVertexArray", __FILE__);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    dump_opengl_errors("glBindBuffer", __FILE__);
    // TODO: Figure out if GL_DYNAMIC_DRAW is the best enum to use or there's something better.
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_PUN) * num_vertices_used, chunk_vertices, GL_DYNAMIC_DRAW);
    dump_opengl_errors("glBufferData", __FILE__);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PUN),
                          (const void*)offsetof(Vertex_PUN, position));
    dump_opengl_errors("glVertexAttribPointer", __FILE__);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_PUN),
                          (const void*)offsetof(Vertex_PUN, tex_coords));
    dump_opengl_errors("glVertexAttribPointer", __FILE__);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PUN),
                          (const void*)offsetof(Vertex_PUN, normal));
    dump_opengl_errors("glVertexAttribPointer", __FILE__);
    glEnableVertexAttribArray(2);

    glDrawArrays(GL_TRIANGLES, 0, num_vertices_used);
    dump_opengl_errors("glDrawArrays", __FILE__);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
render_world(const World &world)
{
    // Assuming that every chunk uses the same shader program.
    for (i32 chunk_xi = 0; chunk_xi < World::NUM_CHUNKS_PER_AXIS; chunk_xi++)
    {
        for (i32 chunk_yi = 0; chunk_yi < World::NUM_CHUNKS_PER_AXIS; chunk_yi++)
        {
            for (i32 chunk_zi = 0; chunk_zi < World::NUM_CHUNKS_PER_AXIS; chunk_zi++)
            {
                Chunk chunk = world.chunks[chunk_xi][chunk_yi][chunk_zi];
                render_chunk(world, chunk);
            }
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
render_loading_screen()
{
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
