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
#include "font.hpp"

lt_global_variable lt::Logger logger("renderer");

struct Vertex_PUN
{
    Vec3f position;
    Vec2f tex_coords;
    Vec3f normal;
};

struct Vertex_PLN
{
    Vec3f position;
    Vec3f tex_coords_layer;
    Vec3f normal;
};

lt_internal inline Vec3f
get_world_coords(const Chunk &chunk, i32 block_xi, i32 block_yi, i32 block_zi)
{
    Vec3f coords;
    coords.x = chunk.origin.x + (block_xi * Chunk::BLOCK_SIZE);
    coords.y = chunk.origin.y + (block_yi * Chunk::BLOCK_SIZE);
    coords.z = chunk.origin.z + (block_zi * Chunk::BLOCK_SIZE);
    return coords;
}

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
render_chunk(const World &world, i32 cx, i32 cy, i32 cz)
{
    const Vec3f pos_x_offset(Chunk::BLOCK_SIZE, 0, 0);
    const Vec3f pos_y_offset(0, Chunk::BLOCK_SIZE, 0);
    const Vec3f pos_z_offset(0, 0, Chunk::BLOCK_SIZE);

    constexpr i32 MAX_NUM_VERTICES = Chunk::NUM_BLOCKS * 36;
    lt_local_persist Vertex_PLN chunk_vertices[MAX_NUM_VERTICES];

    // Set the array to zero for each call of the function.
    std::memset(chunk_vertices, 0, sizeof(chunk_vertices));

    i32 num_vertices_used = 0; // number of vertices used in the array.
    const Chunk chunk = world.chunks[cx][cy][cz];

    for (i32 bx = 0; bx < Chunk::NUM_BLOCKS_PER_AXIS; bx++)
        for (i32 by = 0; by < Chunk::NUM_BLOCKS_PER_AXIS; by++)
            for (i32 bz = 0; bz < Chunk::NUM_BLOCKS_PER_AXIS; bz++)
            {
                // FIXME: This is slow.
                // apply first face merging to see if it has benefits.
                // if (!world.camera.frustum.is_chunk_inside(chunk))
                //     continue;

                // Absolute indexes for the block.
                const i32 abx = cx*Chunk::NUM_BLOCKS_PER_AXIS + bx;
                const i32 aby = cy*Chunk::NUM_BLOCKS_PER_AXIS + by;
                const i32 abz = cz*Chunk::NUM_BLOCKS_PER_AXIS + bz;

                const BlockType block_type = chunk.blocks[bx][by][bz];
                if (block_type == BlockType_Air)
                    continue;

                LT_Assert(block_type >= 0 && block_type < BlockType_Count);

                // Where the block is located in world space.
                const Vec3f block_origin = get_world_coords(chunk, bx, by, bz);

                // All possible vertices of the block.
                const Vec3f left_bottom_back = block_origin;
                const Vec3f left_bottom_front = left_bottom_back + pos_z_offset;
                const Vec3f left_top_front = left_bottom_front + pos_y_offset;
                const Vec3f left_top_back = left_top_front - pos_z_offset;
                const Vec3f right_bottom_back = left_bottom_back + pos_x_offset;
                const Vec3f right_top_back = right_bottom_back + pos_y_offset;
                const Vec3f right_top_front = right_top_back + pos_z_offset;
                const Vec3f right_bottom_front = right_top_front - pos_y_offset;

                // Check faces to render
                const bool should_render_left_face =
                    (abx == 0) ||
                    !world.block_exists(abx-1, aby, abz);

                const bool should_render_right_face =
                    (abx == World::TOTAL_BLOCKS_X-1) ||
                    !world.block_exists(abx+1, aby, abz);

                const bool should_render_top_face =
                    (aby == World::TOTAL_BLOCKS_Y-1) ||
                    !world.block_exists(abx, aby+1, abz);

                const bool should_render_front_face =
                    (abz == World::TOTAL_BLOCKS_Z-1) ||
                    !world.block_exists(abx, aby, abz+1);

                const bool should_render_bottom_face =
                    (aby == 0) ||
                    !world.block_exists(abx, aby-1, abz);

                const bool should_render_back_face =
                    (abz == 0) ||
                    !world.block_exists(abx, aby, abz-1);

                const i32 sides_layer = (should_render_top_face)
                    ? BlocksTextureInfo::Sides_Top_Uncovered
                    : BlocksTextureInfo::Sides_Top_Covered;

                // Vec2f sides_uv_offset(0.5f, 0.0f);

                // Left face ------------------------------------------------------
                if (should_render_left_face)
                {
                    chunk_vertices[num_vertices_used].position   = left_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = left_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = left_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = left_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = left_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = left_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(-1, 0, 0);
                }
                // Right face -----------------------------------------------------
                if (should_render_right_face)
                {
                    chunk_vertices[num_vertices_used].position   = right_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = right_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = right_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal     = Vec3f(1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = right_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal     = Vec3f(1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = right_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);

                    chunk_vertices[num_vertices_used].position   = right_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(1, 0, 0);
                }
                // Top face --------------------------------------------------------
                if (should_render_top_face)
                {
                    const i32 top_layer = BlocksTextureInfo::Top;
                    chunk_vertices[num_vertices_used].position   = left_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, top_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                    chunk_vertices[num_vertices_used].position   = right_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 0, top_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                    chunk_vertices[num_vertices_used].position   = right_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, top_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                    chunk_vertices[num_vertices_used].position   = right_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, top_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                    chunk_vertices[num_vertices_used].position   = left_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 1, top_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);

                    chunk_vertices[num_vertices_used].position   = left_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, top_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 1, 0);
                }
                // Bottom face -----------------------------------------------------
                if (should_render_bottom_face)
                {
                    const i32 bottom_layer = BlocksTextureInfo::Bottom;
                    chunk_vertices[num_vertices_used].position   = left_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, bottom_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                    chunk_vertices[num_vertices_used].position   = right_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 0, bottom_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                    chunk_vertices[num_vertices_used].position   = right_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, bottom_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                    chunk_vertices[num_vertices_used].position   = right_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, bottom_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                    chunk_vertices[num_vertices_used].position   = left_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 1, bottom_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);

                    chunk_vertices[num_vertices_used].position   = left_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, bottom_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, -1, 0);
                }
                // Front face -------------------------------------------------------
                if (should_render_front_face)
                {
                    chunk_vertices[num_vertices_used].position   = left_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                    chunk_vertices[num_vertices_used].position   = right_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                    chunk_vertices[num_vertices_used].position   = right_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                    chunk_vertices[num_vertices_used].position   = right_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                    chunk_vertices[num_vertices_used].position   = left_top_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);

                    chunk_vertices[num_vertices_used].position   = left_bottom_front;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, 1);
                }
                // Back face --------------------------------------------------------
                if (should_render_back_face)
                {
                    chunk_vertices[num_vertices_used].position   = right_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                    chunk_vertices[num_vertices_used].position   = left_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                    chunk_vertices[num_vertices_used].position   = left_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                    chunk_vertices[num_vertices_used].position   = left_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                    chunk_vertices[num_vertices_used].position   = right_top_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);

                    chunk_vertices[num_vertices_used].position   = right_bottom_back;
                    chunk_vertices[num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                    chunk_vertices[num_vertices_used++].normal   = Vec3f(0, 0, -1);
                }
            }

    LT_Assert(num_vertices_used <= MAX_NUM_VERTICES);
    LT_Assert(chunk.vbo != 0); // The vbo should already be created.

    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    // TODO: Figure out if GL_DYNAMIC_DRAW is the best enum to use or there's something better.
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_PLN) * num_vertices_used, chunk_vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                          (const void*)offsetof(Vertex_PLN, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                          (const void*)offsetof(Vertex_PLN, tex_coords_layer));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                          (const void*)offsetof(Vertex_PLN, normal));
    glEnableVertexAttribArray(2);

    glDrawArrays(GL_TRIANGLES, 0, num_vertices_used);
    dump_opengl_errors("glDrawArrays", __FILE__);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
render_world(const World &world)
{
    // Assuming that every chunk uses the same shader program.
    for (i32 cx = 0; cx < World::NUM_CHUNKS_X; cx++)
        for (i32 cy = 0; cy < World::NUM_CHUNKS_Y; cy++)
            for (i32 cz = 0; cz < World::NUM_CHUNKS_Z; cz++)
                render_chunk(world, cx, cy, cz);
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
