#include "landscape.hpp"
#include "open-simplex-noise.h"
#include "lt_utils.hpp"
#include "gl_resources.hpp"
#include "texture.hpp"
#include "resource_manager.hpp"
#include "vertex.hpp"
#include "application.hpp"
#include "camera.hpp"
//#include "pool_allocator.hpp"

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

Landscape::Landscape(Memory &memory, i32 seed, f64 amplitude,
                     f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain)
    // : chunks(PoolAllocator<Chunk>(memory.chunks_memory, memory.chunks_memory_size, sizeof(Chunk), alignof(Chunk)))
    : chunks()
    , origin(Vec3f(0))
    , m_seed(seed)
    , m_amplitude(amplitude)
    , m_frequency(frequency)
    , m_num_octaves(num_octaves)
    , m_lacunarity(lacunarity)
    , m_gain(gain)
{
    initialize_chunks();

    if (open_simplex_noise(seed, &m_simplex_ctx))
    {
        logger.error("Failed to initialize context for noise generation.");
        return;
    }
}

Vec3f
Landscape::get_chunk_origin(i32 cx, i32 cy, i32 cz)
{
    return origin + Vec3f(cx*Chunk::SIZE, cy*Chunk::SIZE, cz*Chunk::SIZE);
}

void
Landscape::update(const Camera &camera)
{
    const f32 x_distance_to_center = camera.position().x - center().x;
    const f32 z_distance_to_center = camera.position().z - center().z;

    // NOTE: Ignore the y axis for the moment.
    const f32 chosen_distance = 1*Chunk::SIZE;

    if (x_distance_to_center > chosen_distance) // positive x
    {
        origin.x += chosen_distance;

        for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
            for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
                {
                    auto chunk_it = chunk_ptrs[cx][cy][cz];
                    if (cx > 0)
                    {
                        chunk_ptrs[cx-1][cy][cz] = chunk_it;

                        if (cx == NUM_CHUNKS_X-1)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                            chunks.emplace_front(chunk_origin);
                            chunk_ptrs[cx][cy][cz] = chunks.begin();
                            generate_for_chunk(cx, cy, cz);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunks.erase(chunk_it);
                    }
                }
    }
    else if (x_distance_to_center < -chosen_distance) // negative x
    {
        origin.x -= chosen_distance;

        for (i32 cx = NUM_CHUNKS_X-1; cx >= 0; cx--)
            for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
                {
                    auto chunk_it = chunk_ptrs[cx][cy][cz];
                    if (cx < NUM_CHUNKS_X-1)
                    {
                        chunk_ptrs[cx+1][cy][cz] = chunk_it;

                        if (cx == 0)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                            chunks.emplace_front(chunk_origin);
                            chunk_ptrs[cx][cy][cz] = chunks.begin();
                            generate_for_chunk(cx, cy, cz);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunks.erase(chunk_it);
                    }
                }
    }

    if (z_distance_to_center > chosen_distance) // positive z
    {
        origin.z += chosen_distance;

        for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
            for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
                for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                {
                    auto chunk_it = chunk_ptrs[cx][cy][cz];
                    if (cz > 0)
                    {
                        chunk_ptrs[cx][cy][cz-1] = chunk_it;

                        if (cz == NUM_CHUNKS_Z-1)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                            chunks.emplace_front(chunk_origin);
                            chunk_ptrs[cx][cy][cz] = chunks.begin();
                            generate_for_chunk(cx, cy, cz);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunks.erase(chunk_it);
                    }
                }
    }
    else if (z_distance_to_center < -chosen_distance) // negative z
    {
        origin.z -= chosen_distance;

        for (i32 cz = NUM_CHUNKS_Z-1; cz >= 0; cz--)
            for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
                for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                {
                    auto chunk_it = chunk_ptrs[cx][cy][cz];
                    if (cz < NUM_CHUNKS_Z-1)
                    {
                        chunk_ptrs[cx][cy][cz+1] = chunk_it;

                        if (cz == 0)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                            chunks.emplace_front(chunk_origin);
                            chunk_ptrs[cx][cy][cz] = chunks.begin();
                            generate_for_chunk(cx, cy, cz);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunks.erase(chunk_it);
                    }
                }
    }

    for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
        for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
            for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
            {
                if (chunk_ptrs[cx][cy][cz]->outdated)
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

    return chunk_ptrs[cx][cy][cz]->blocks[bx][by][bz] != BlockType_Air;
}


void
Landscape::initialize_chunks()
{
    logger.log("Initialize buffers called.");
    for (i32 x = 0; x < NUM_CHUNKS_X; x++)
        for (i32 y = 0; y < NUM_CHUNKS_Y; y++)
            for (i32 z = 0; z < NUM_CHUNKS_Z; z++)
            {
                const Vec3f chunk_origin = get_chunk_origin(x, y, z);
                chunks.emplace_front(chunk_origin);

                auto chunk_it = chunks.begin();

                chunk_ptrs[x][y][z] = chunk_it;
            }

    LT_Assert(chunks.size() == NUM_CHUNKS_X*NUM_CHUNKS_Y*NUM_CHUNKS_Z);
}

void
Landscape::update_chunk_buffer(i32 cx, i32 cy, i32 cz)
{
    Chunk &chunk = *chunk_ptrs[cx][cy][cz];

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
                        ? BlocksTextureInfo::Snow_Sides_Top
                        : BlocksTextureInfo::Snow_Sides;

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
                        const i32 top_layer = BlocksTextureInfo::Snow_Top;
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
                        const i32 bottom_layer = BlocksTextureInfo::Snow_Bottom;
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
        LT_Assert(chunk.vao != 0); // The vbo should already be created.

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
    else
    {
        logger.log("Chunk ", cx, " ", cy, " ", cz, " is not outdated.");
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

struct ChunkNoise
{
    f64 map[Chunk::NUM_BLOCKS_PER_AXIS][Chunk::NUM_BLOCKS_PER_AXIS];
};

void
Landscape::generate_for_chunk(i32 cx, i32 cy, i32 cz)
{
    ChunkNoise *chunk_noise = fill_noise_map_for_chunk_column(cx, cz);

    for (i32 bx = 0; bx < Chunk::NUM_BLOCKS_PER_AXIS; bx++)
        for (i32 bz = 0; bz < Chunk::NUM_BLOCKS_PER_AXIS; bz++)
        {
            const f64 normalized_height = chunk_noise->map[bz][bx];
            const i32 height_aby = std::round((f64)(TOTAL_BLOCKS_Y-1) * normalized_height);
            const i32 height_by = height_aby % Chunk::NUM_BLOCKS_PER_AXIS;
            const i32 height_cy = height_aby / Chunk::NUM_BLOCKS_PER_AXIS;

            if (height_cy > cy)
            {
                for (i32 by = 0; by < Chunk::NUM_BLOCKS_PER_AXIS; by++)
                    chunk_ptrs[cx][cy][cz]->blocks[bx][by][bz] = BlockType_Terrain;
            }
            else if (height_cy == cy)
            {
                for (i32 by = 0; by <= height_by; by++)
                    chunk_ptrs[cx][cy][cz]->blocks[bx][by][bz] = BlockType_Terrain;
            }
        }
}

ChunkNoise *
Landscape::fill_noise_map_for_chunk_column(i32 cx, i32 cz)
{
    //
    // FIXME: This function is not thread safe, since it returns a local persisted
    // buffer for the noise map.
    //
    lt_internal ChunkNoise chunk_noise = {};

    //auto chunk_it = chunk_ptrs[cx][0][cz];
    Chunk &chunk = *chunk_ptrs[cx][0][cz];

    const f32 origin_x = chunk.origin.x;
    const f32 origin_z = chunk.origin.z;

    // Fill out noise_map
    for (i32 z = 0; z < Chunk::NUM_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < Chunk::NUM_BLOCKS_PER_AXIS; x++)
        {
            const f64 noise_x = (f64)origin_x + Chunk::BLOCK_SIZE*x;
            const f64 noise_y = (f64)origin_z + Chunk::BLOCK_SIZE*z;
            const f64 noise = get_fbm(m_simplex_ctx, noise_x, noise_y, m_amplitude,
                                      m_frequency, m_num_octaves, m_lacunarity, m_gain);
            chunk_noise.map[z][x] = noise;
        }

    // Rescale noise values into 0 -> 1 range
    const f64 min = -1.0;
    const f64 max = 1.0;

    // // - Actually rescale the values
    for (i32 z = 0; z < Chunk::NUM_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < Chunk::NUM_BLOCKS_PER_AXIS; x++)
        {
            chunk_noise.map[z][x] = (chunk_noise.map[z][x] + (max - min)) / (2*max - min);

            const f64 EPSILON = 0.001;
            LT_Assert(chunk_noise.map[z][x] <= (max+EPSILON) && chunk_noise.map[z][x] >= (min-EPSILON));

            LT_Assert(chunk_noise.map[z][x] <= (1.0 + EPSILON) && chunk_noise.map[z][x] >= -EPSILON);
        }

    return &chunk_noise;
}

void
Landscape::generate()
{
    logger.log("Generating landscape");

    for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
        for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
        {
            ChunkNoise *chunk_noise = fill_noise_map_for_chunk_column(cx, cz);

            for (i32 bx = 0; bx < Chunk::NUM_BLOCKS_PER_AXIS; bx++)
                for (i32 bz = 0; bz < Chunk::NUM_BLOCKS_PER_AXIS; bz++)
                {
                    const f64 normalized_height = chunk_noise->map[bz][bx];
                    const i32 abs_block_y = std::round((f64)(TOTAL_BLOCKS_Y-1) * normalized_height);

                    for (i32 aby = 0; aby <= abs_block_y; aby++)
                    {
                        const i32 cy = aby / Chunk::NUM_BLOCKS_PER_AXIS;
                        const i32 by = aby % Chunk::NUM_BLOCKS_PER_AXIS;

                        chunk_ptrs[cx][cy][cz]->blocks[bx][by][bz] = BlockType_Terrain;
                    }
                }
        }
}


// ----------------------------------------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------------------------------------

Chunk::Chunk(Vec3f origin, BlockType fill_type)
    : origin(origin)
    , vao(GLResources::instance().create_vertex_array())
    , vbo(GLResources::instance().create_buffer())
    , outdated(true) // starts outdated so it can be initially rendered as the game starts.
{
    for (i32 x = 0; x < NUM_BLOCKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_BLOCKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_BLOCKS_PER_AXIS; z++)
                blocks[x][y][z] = fill_type;

    LT_Assert(vao > 0);
    LT_Assert(vbo > 0);
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
