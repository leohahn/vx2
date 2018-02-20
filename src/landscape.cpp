#include "landscape.hpp"
#include "open-simplex-noise.h"
#include "lt_utils.hpp"
#include "gl_resources.hpp"
#include "texture.hpp"
#include "resource_manager.hpp"
#include "vertex.hpp"
#include "application.hpp"

lt_global_variable lt::Logger logger("landscape");

lt_internal inline Vec3f
get_world_coords(const Chunk &chunk, i32 block_xi, i32 block_yi, i32 block_zi)
{
    Vec3f coords;
    coords.x = chunk.origin.x + (block_xi * Chunk::BLOCK_SIZE);
    coords.y = chunk.origin.y + (block_yi * Chunk::BLOCK_SIZE);
    coords.z = chunk.origin.z + (block_zi * Chunk::BLOCK_SIZE);
    return coords;
}

Landscape::Landscape(i32 seed)
    : origin(Vec3f(0))
    , m_seed(seed)
{
    initialize_buffers();

    if (open_simplex_noise(seed, &m_simplex_ctx))
    {
        logger.error("Failed to initialize context for noise generation.");
        return;
    }
}

void
Landscape::update()
{
    for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
        for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
            for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
            {
                if (chunks[cx][cy][cz].outdated)
                    update_chunk_buffer(cx, cy, cz);
            }
}

bool
Landscape::block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi) const
{
    const i32 bx = abs_block_xi % Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 by = abs_block_yi % Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 bz = abs_block_zi % Chunk::NUM_BLOCKS_PER_AXIS;

    const i32 cx = abs_block_xi / Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 cy = abs_block_yi / Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 cz = abs_block_zi / Chunk::NUM_BLOCKS_PER_AXIS;

    return chunks[cx][cy][cz].blocks[bx][by][bz] != BlockType_Air;
}


void
Landscape::initialize_buffers()
{
    logger.log("Initialize buffers called.");
    for (i32 x = 0; x < NUM_CHUNKS_X; x++)
        for (i32 y = 0; y < NUM_CHUNKS_Y; y++)
            for (i32 z = 0; z < NUM_CHUNKS_Z; z++)
            {
                const f32 offset = Chunk::BLOCK_SIZE * Chunk::NUM_BLOCKS_PER_AXIS;
                const Vec3f chunk_origin = origin + Vec3f(x*offset, y*offset, z*offset);
                chunks[x][y][z].origin = chunk_origin;
                chunks[x][y][z].vao = GLResources::instance().create_vertex_array();
                chunks[x][y][z].vbo = GLResources::instance().create_buffer();
                const f32 chunk_offset = Chunk::BLOCK_SIZE * Chunk::NUM_BLOCKS_PER_AXIS;
                chunks[x][y][z].max_vertices[0] = chunk_origin;
                chunks[x][y][z].max_vertices[1] = chunk_origin + Vec3f(0.0f,         0.0f,         chunk_offset);
                chunks[x][y][z].max_vertices[2] = chunk_origin + Vec3f(0.0f,         chunk_offset, 0.0f);
                chunks[x][y][z].max_vertices[3] = chunk_origin + Vec3f(chunk_offset, 0.0f,         0.0f);
                chunks[x][y][z].max_vertices[4] = chunk_origin + Vec3f(0.0f,         chunk_offset, chunk_offset);
                chunks[x][y][z].max_vertices[5] = chunk_origin + Vec3f(chunk_offset, chunk_offset, 0.0f);
                chunks[x][y][z].max_vertices[6] = chunk_origin + Vec3f(chunk_offset, 0.0f,         chunk_offset);
                chunks[x][y][z].max_vertices[7] = chunk_origin + Vec3f(chunk_offset, chunk_offset, chunk_offset);
            }
}

void
Landscape::update_chunk_buffer(i32 cx, i32 cy, i32 cz)
{
    Chunk &chunk = chunks[cx][cy][cz];

    // If chunk is not outdated, avoid updating the vertexes.
    if (chunk.outdated)
    {
        const Vec3f pos_x_offset(Chunk::BLOCK_SIZE, 0, 0);
        const Vec3f pos_y_offset(0, Chunk::BLOCK_SIZE, 0);
        const Vec3f pos_z_offset(0, 0, Chunk::BLOCK_SIZE);

        constexpr i32 MAX_NUM_VERTICES = Chunk::NUM_BLOCKS * 36;
        lt_local_persist Vertex_PLN chunk_vertices[MAX_NUM_VERTICES];

        // Set the array to zero for each call of the function.
        std::memset(chunk_vertices, 0, sizeof(chunk_vertices));

        chunk.num_vertices_used = 0; // number of vertices used in the array.

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
                        !block_exists(abx-1, aby, abz);

                    const bool should_render_right_face =
                        (abx == TOTAL_BLOCKS_X-1) ||
                        !block_exists(abx+1, aby, abz);

                    const bool should_render_top_face =
                        (aby == TOTAL_BLOCKS_Y-1) ||
                        !block_exists(abx, aby+1, abz);

                    const bool should_render_front_face =
                        (abz == TOTAL_BLOCKS_Z-1) ||
                        !block_exists(abx, aby, abz+1);

                    const bool should_render_bottom_face =
                        (aby == 0) ||
                        !block_exists(abx, aby-1, abz);

                    const bool should_render_back_face =
                        (abz == 0) ||
                        !block_exists(abx, aby, abz-1);

                    const i32 sides_layer = (should_render_top_face)
                        ? BlocksTextureInfo::Sides_Top_Uncovered
                        : BlocksTextureInfo::Sides_Top_Covered;

                    // Vec2f sides_uv_offset(0.5f, 0.0f);

                    // Left face ------------------------------------------------------
                    if (should_render_left_face)
                    {
                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(-1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(-1, 0, 0);
                    }
                    // Right face -----------------------------------------------------
                    if (should_render_right_face)
                    {
                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal     = Vec3f(1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal     = Vec3f(1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(1, 0, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(1, 0, 0);
                    }
                    // Top face --------------------------------------------------------
                    if (should_render_top_face)
                    {
                        const i32 top_layer = BlocksTextureInfo::Top;
                        chunk_vertices[chunk.num_vertices_used].position   = left_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, top_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 0, top_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, top_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, top_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 1, top_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, top_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 1, 0);
                    }
                    // Bottom face -----------------------------------------------------
                    if (should_render_bottom_face)
                    {
                        const i32 bottom_layer = BlocksTextureInfo::Bottom;
                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, bottom_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, -1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 0, bottom_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, -1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, bottom_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, -1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, bottom_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, -1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 1, bottom_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, -1, 0);

                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, bottom_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, -1, 0);
                    }
                    // Front face -------------------------------------------------------
                    if (should_render_front_face)
                    {
                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, 1);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, 1);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, 1);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, 1);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, 1);

                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_front;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, 1);
                    }
                    // Back face --------------------------------------------------------
                    if (should_render_back_face)
                    {
                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, -1);

                        chunk_vertices[chunk.num_vertices_used].position   = left_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, -1);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, -1);

                        chunk_vertices[chunk.num_vertices_used].position   = left_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(1, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, -1);

                        chunk_vertices[chunk.num_vertices_used].position   = right_top_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 1, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, -1);

                        chunk_vertices[chunk.num_vertices_used].position   = right_bottom_back;
                        chunk_vertices[chunk.num_vertices_used].tex_coords_layer = Vec3f(0, 0, sides_layer);
                        chunk_vertices[chunk.num_vertices_used++].normal   = Vec3f(0, 0, -1);
                    }
                }

        LT_Assert(chunk.num_vertices_used <= MAX_NUM_VERTICES);
        LT_Assert(chunk.vbo != 0); // The vbo should already be created.

        glBindVertexArray(chunk.vao);
        glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
        // TODO: Figure out if GL_DYNAMIC_DRAW is the best enum to use or there's something better.
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_PLN) * chunk.num_vertices_used,
                     chunk_vertices, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                              (const void*)offsetof(Vertex_PLN, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                              (const void*)offsetof(Vertex_PLN, tex_coords_layer));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                              (const void*)offsetof(Vertex_PLN, normal));
        glEnableVertexAttribArray(2);

        glDrawArrays(GL_TRIANGLES, 0, chunk.num_vertices_used);
        dump_opengl_errors("glDrawArrays", __FILE__);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        chunk.outdated = false;
    }
}

lt_internal f64
get_fbm(struct osn_context *ctx, f64 x, f64 y, f64 amplitude,
        f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain)
{
    f64 fbm = 0.0;
    for (i32 i = 0; i < num_octaves; i++)
    {
        fbm += amplitude * open_simplex_noise2(ctx, frequency*x, frequency*y);
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return fbm;
}

void
Landscape::generate(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain)
{
    logger.log("Generating landscape");

    f64 noise_map[TOTAL_BLOCKS_X*TOTAL_BLOCKS_Z] = {};

    // Fill out noise_map
    for (i32 z = 0; z < TOTAL_BLOCKS_Z; z++)
        for (i32 x = 0; x < TOTAL_BLOCKS_X; x++)
        {
            const f64 noise_x = (f64)x;
            const f64 noise_y = (f64)z;
            f64 noise = get_fbm(m_simplex_ctx, noise_x, noise_y, amplitude,
                                frequency, num_octaves, lacunarity, gain);
            noise_map[x + z*TOTAL_BLOCKS_X] = noise;
        }

    //
    // Rescale noise values into 0 -> 1 range
    //
    // - Find first the maximum and minimum values
    f64 min = std::numeric_limits<f64>::max();
    f64 max = std::numeric_limits<f64>::min();
    for (i32 z = 0; z < TOTAL_BLOCKS_Z; z++)
        for (i32 x = 0; x < TOTAL_BLOCKS_X; x++)
        {
            const i32 i = x + z*TOTAL_BLOCKS_X;
            if (noise_map[i] > max)
                max = noise_map[i];
            if (noise_map[i] < min)
                min = noise_map[i];
        }

    // - Actually rescale the values
    for (i32 z = 0; z < TOTAL_BLOCKS_Z; z++)
        for (i32 x = 0; x < TOTAL_BLOCKS_X; x++)
        {
            const i32 i = x + z*TOTAL_BLOCKS_X;

            f64 noise = noise_map[i];
            noise_map[i] = (noise + (max - min)) / (2*max - min);

            const f64 EPSILON = 0.002;
            LT_Assert(noise_map[i] <= (1.0 + EPSILON) && noise_map[i] >= -EPSILON);
        }

    for (i32 abs_block_x = 0; abs_block_x < TOTAL_BLOCKS_X; abs_block_x++)
        for (i32 abs_block_z = 0; abs_block_z < TOTAL_BLOCKS_Z; abs_block_z++)
        {
            const i32 block_xi = abs_block_x % Chunk::NUM_BLOCKS_PER_AXIS;
            const i32 block_zi = abs_block_z % Chunk::NUM_BLOCKS_PER_AXIS;

            const i32 chunk_xi = abs_block_x / Chunk::NUM_BLOCKS_PER_AXIS;
            const i32 chunk_zi = abs_block_z / Chunk::NUM_BLOCKS_PER_AXIS;

            const i32 noise_index = abs_block_x + abs_block_z*TOTAL_BLOCKS_X;
            const f64 height = noise_map[noise_index];

            const i32 abs_block_y = std::round((f64)(TOTAL_BLOCKS_Y-1) * height);
            // const i32 abs_block_y = (TOTAL_BLOCKS_Y-1);

            for (i32 y = 0; y <= abs_block_y; y++)
            {
                const i32 block_yi = y % Chunk::NUM_BLOCKS_PER_AXIS;
                const i32 chunk_yi = y / Chunk::NUM_BLOCKS_PER_AXIS;

                Chunk &chunk = chunks[chunk_xi][chunk_yi][chunk_zi];
                chunk.blocks[block_xi][block_yi][block_zi] = BlockType_Terrain;
            }
        }
}


// ----------------------------------------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------------------------------------

Chunk::Chunk()
    : vao(0)
    , vbo(0)
    , outdated(true) // starts outdated so it can be initially rendered as the game starts.
{
    for (i32 x = 0; x < NUM_BLOCKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_BLOCKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_BLOCKS_PER_AXIS; z++)
                blocks[x][y][z] = BlockType_Air;
}

Chunk::~Chunk()
{
    GLResources::instance().delete_vertex_array(vao);
    GLResources::instance().delete_buffer(vbo);
}

// ----------------------------------------------------------------------------------------------
// Blocks Texture Info
// ----------------------------------------------------------------------------------------------

BlocksTextureInfo::BlocksTextureInfo(const char *texture_name, const ResourceManager &manager)
    : m_texture(manager.get_texture<TextureAtlas>(texture_name))
{
    if (!m_texture)
        logger.error("Failed to get texture ", texture_name);
}

bool BlocksTextureInfo::load() { return m_texture->load(); }

u32 BlocksTextureInfo::texture_id() const { return m_texture->id; }
