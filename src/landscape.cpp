#include "landscape.hpp"
#include "open-simplex-noise.h"
#include "lt_utils.hpp"
#include "gl_resources.hpp"
#include "texture.hpp"
#include "resource_manager.hpp"
#include "application.hpp"
#include "camera.hpp"
#include "world.hpp"
#include "input.hpp"

using std::placeholders::_1;

lt_global_variable lt::Logger logger("landscape");

lt_internal inline Vec3f
get_world_coords(const Landscape::Chunk &chunk, i32 block_xi, i32 block_yi, i32 block_zi)
{
    Vec3f coords;
    coords.x = chunk.origin.x + (block_xi * Landscape::Chunk::BLOCK_SIZE);
    coords.y = chunk.origin.y + (block_yi * Landscape::Chunk::BLOCK_SIZE);
    coords.z = chunk.origin.z + (block_zi * Landscape::Chunk::BLOCK_SIZE);
    return coords;
}

Landscape::Landscape(Memory &memory, i32 seed, f64 amplitude,
                     f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain)
    : origin(Vec3f(0))
    , m_seed(seed)
    , m_amplitude(amplitude)
    , m_frequency(frequency)
    , m_num_octaves(num_octaves)
    , m_lacunarity(lacunarity)
    , m_gain(gain)
    , m_threads_should_run(false)
    , m_chunks_allocator(memory.chunks_memory, memory.chunks_memory_size,
                         sizeof(Chunk), alignof(Chunk))
{
    const u32 num_cpu_cores = std::thread::hardware_concurrency();
    if (num_cpu_cores == 0)
        LT_Panic("Could not find the number of cores in this computer.");
    else if (num_cpu_cores == 1)
        LT_Panic("This computer only has one core.");
    else
    {
        // NOTE: One thread should be reserved for the main thread.
        m_threads = std::vector<std::thread>(num_cpu_cores-1);
    }

    if (open_simplex_noise(seed, &m_simplex_ctx))
        LT_Panic("Failed to initialize context for noise generation.");

    initialize_chunks();
    initialize_threads();
}

Landscape::~Landscape()
{
    stop_threads();
    open_simplex_noise_free(m_simplex_ctx);
}

Vec3f
Landscape::get_chunk_origin(i32 cx, i32 cy, i32 cz)
{
    const Vec3f new_origin = origin + Vec3f(cx*Chunk::SIZE, cy*Chunk::SIZE, cz*Chunk::SIZE);
    return new_origin;
}

void
Landscape::update(const Camera &camera, const Input &input)
{
    // See which chunks where already loaded by the different threads and
    // pass the vertex data to the gpu if so happened.
    {
        const i32 MAX_CHUNKS_TO_LOAD = 5;

        i32 chunks_loaded = 0;

        for (auto &queue : m_chunks_processed_queues)
        {
            std::shared_ptr<QueueRequest> request = nullptr;
            while (chunks_loaded++ < MAX_CHUNKS_TO_LOAD && (request = queue.take_next_request()))
            {
                LT_Assert(request->processed);

                if (request->chunk)
                {
                    chunks_mutex.lock_high_priority(); // LOCK

                    auto &entry = vao_array.vaos[request->chunk->entry_index];
                    entry.num_vertices_used = request->vertexes.size();

                    chunks_mutex.unlock_high_priority(); // UNLOCK

                    if (entry.num_vertices_used > 0)
                        pass_chunk_buffer_to_gpu(entry, request->vertexes);
                }
            }
        }
    }

    const f32 x_distance_to_center = camera.position().x - center().x;
    const f32 z_distance_to_center = camera.position().z - center().z;

    // NOTE: Ignore the y axis for the moment.
    const f32 chosen_distance = 1*Chunk::SIZE;

    if (x_distance_to_center > chosen_distance) // positive x
    {
        chunks_mutex.lock_high_priority(); // LOCK
        origin.x += chosen_distance;

        for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
            for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
                {
                    if (cx > 0)
                    {
                        chunk_ptrs[cx-1][cy][cz] = std::move(chunk_ptrs[cx][cy][cz]);
                        LT_Assert(chunk_ptrs[cx-1][cy][cz]);

                        if (cx == NUM_CHUNKS_X-1)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);

                            Chunk *chunk = memory::allocate_and_construct<Chunk>(
                                m_chunks_allocator, chunk_origin, &vao_array
                            );
                            LT_Assert(chunk);
                            chunk_ptrs[cx][cy][cz] = ChunkPtr(
                                chunk, std::bind(&Landscape::chunk_deleter, this, _1)
                            );
                            LT_Assert(chunk_ptrs[cx][cy][cz]);

                            do_chunk_generation_work(chunk);

                            chunk->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk->request,
                                                                      &m_chunks_to_process_semaphore);

                            chunk_ptrs[cx-1][cy][cz]->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk_ptrs[cx-1][cy][cz]->request,
                                                                      &m_chunks_to_process_semaphore);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunk_ptrs[cx][cy][cz]->cancel_request();
                        chunk_ptrs[cx][cy][cz].reset();
                    }
                }
        chunks_mutex.unlock_high_priority(); // UNLOCK
    }
    else if (x_distance_to_center < -chosen_distance) // negative x
    {
        chunks_mutex.lock_high_priority(); // LOCK
        origin.x -= chosen_distance;

        for (i32 cx = NUM_CHUNKS_X-1; cx >= 0; cx--)
            for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
                {
                    if (cx < NUM_CHUNKS_X-1)
                    {
                        chunk_ptrs[cx+1][cy][cz] = std::move(chunk_ptrs[cx][cy][cz]);
                        LT_Assert(chunk_ptrs[cx+1][cy][cz]);

                        if (cx == 0)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);

                            Chunk *chunk = memory::allocate_and_construct<Chunk>(
                                m_chunks_allocator, chunk_origin, &vao_array
                            );
                            chunk_ptrs[cx][cy][cz] = ChunkPtr(
                                chunk, std::bind(&Landscape::chunk_deleter, this, _1)
                            );
                            LT_Assert(chunk_ptrs[cx][cy][cz]);

                            do_chunk_generation_work(chunk);
                            chunk->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk->request,
                                                                      &m_chunks_to_process_semaphore);

                            chunk_ptrs[cx+1][cy][cz]->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk_ptrs[cx+1][cy][cz]->request,
                                                                      &m_chunks_to_process_semaphore);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunk_ptrs[cx][cy][cz]->cancel_request();
                        chunk_ptrs[cx][cy][cz].reset();
                    }
                }
        chunks_mutex.unlock_high_priority(); // UNLOCK
    }

    if (z_distance_to_center > chosen_distance) // positive z
    {
        chunks_mutex.lock_high_priority(); // LOCK
        origin.z += chosen_distance;

        for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
            for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
                for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                {
                    if (cz > 0)
                    {
                        chunk_ptrs[cx][cy][cz-1] = std::move(chunk_ptrs[cx][cy][cz]);
                        LT_Assert(chunk_ptrs[cx][cy][cz-1]);

                        if (cz == NUM_CHUNKS_Z-1)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                            Chunk *chunk = memory::allocate_and_construct<Chunk>(
                                m_chunks_allocator, chunk_origin, &vao_array
                            );
                            chunk_ptrs[cx][cy][cz] = ChunkPtr(
                                chunk, std::bind(&Landscape::chunk_deleter, this, _1)
                            );
                            LT_Assert(chunk_ptrs[cx][cy][cz]);

                            do_chunk_generation_work(chunk);
                            chunk->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk->request,
                                                                      &m_chunks_to_process_semaphore);

                            chunk_ptrs[cx][cy][cz-1]->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk_ptrs[cx][cy][cz-1]->request,
                                                                      &m_chunks_to_process_semaphore);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunk_ptrs[cx][cy][cz]->cancel_request();
                        chunk_ptrs[cx][cy][cz].reset();
                    }
                }
        chunks_mutex.unlock_high_priority(); // UNLOCK
    }
    else if (z_distance_to_center < -chosen_distance) // negative z
    {
        chunks_mutex.lock_high_priority(); // LOCK
        origin.z -= chosen_distance;

        for (i32 cz = NUM_CHUNKS_Z-1; cz >= 0; cz--)
            for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
                for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
                {
                    if (cz < NUM_CHUNKS_Z-1)
                    {
                        chunk_ptrs[cx][cy][cz+1] = std::move(chunk_ptrs[cx][cy][cz]);
                        LT_Assert(chunk_ptrs[cx][cy][cz+1]);

                        if (cz == 0)
                        {
                            const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                            Chunk *chunk = memory::allocate_and_construct<Chunk>(
                                m_chunks_allocator, chunk_origin, &vao_array
                            );
                            chunk_ptrs[cx][cy][cz] = ChunkPtr(
                                chunk, std::bind(&Landscape::chunk_deleter, this, _1)
                            );
                            LT_Assert(chunk_ptrs[cx][cy][cz]);

                            do_chunk_generation_work(chunk);
                            chunk->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk->request,
                                                                      &m_chunks_to_process_semaphore);

                            chunk_ptrs[cx][cy][cz+1]->create_request();
                            m_chunks_to_process_queues[QP_Low].insert(chunk_ptrs[cx][cy][cz+1]->request,
                                                                      &m_chunks_to_process_semaphore);
                        }
                    }
                    else
                    {
                        // Remove chunks that are outside of the landscape boundary.
                        // TODO: In the future such chunks should be persisted to disk
                        // or something similar.
                        chunk_ptrs[cx][cy][cz]->cancel_request();
                        chunk_ptrs[cx][cy][cz].reset();
                    }
                }
        chunks_mutex.unlock_high_priority(); // UNLOCK
    }

    if (input.mouse_state.left_button_transition == Transition_Down)
    {
        remove_block(camera.position(), camera.front());
    }
}

bool
Landscape::block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi)
{
    const i32 bx = abs_block_xi % Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 by = abs_block_yi % Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 bz = abs_block_zi % Chunk::NUM_BLOCKS_PER_AXIS;

    const i32 cx = abs_block_xi / Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 cy = abs_block_yi / Chunk::NUM_BLOCKS_PER_AXIS;
    const i32 cz = abs_block_zi / Chunk::NUM_BLOCKS_PER_AXIS;

    // NOTE: the pointer to the chunk should be available. This is necessary in order to
    // optimize the chunk generation.
    LT_Assert(cx < NUM_CHUNKS_X);
    LT_Assert(cy < NUM_CHUNKS_Y);
    LT_Assert(cz < NUM_CHUNKS_Z);
    LT_Assert(chunk_ptrs[cx][cy][cz]);

    return chunk_ptrs[cx][cy][cz]->blocks[bx][by][bz] != BlockType_Air;
}

void
Landscape::initialize_chunks()
{
    logger.log("Initialize chunks called.");

    for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
        for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
            for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
            {
                const Vec3f chunk_origin = get_chunk_origin(cx, cy, cz);
                Chunk *chunk = memory::allocate_and_construct<Chunk>(m_chunks_allocator,
                                                                     chunk_origin, &vao_array);
                chunk_ptrs[cx][cy][cz] = ChunkPtr(
                    chunk, std::bind(&Landscape::chunk_deleter, this, _1)
                );
                do_chunk_generation_work(chunk);
            }
}

std::vector<Vertex_PLN>
Landscape::update_chunk_buffer(Chunk *chunk)
{
    LT_Assert(chunk);

    const i32 cx = (i32)(chunk->origin.x - origin.x) / Chunk::SIZE;
    const i32 cy = (i32)(chunk->origin.y - origin.y) / Chunk::SIZE;
    const i32 cz = (i32)(chunk->origin.z - origin.z) / Chunk::SIZE;

    // PERFORMANCE: In order to improve performance, since many threads will be calling this function,
    // introduce a arena to allocate this vector from. However this introduces complexity by creating
    // a custom allocator with variable block sizes.
    std::vector<Vertex_PLN> chunk_vertices;

    // If chunk is not outdated, avoid updating the vertexes.
    const Vec3f pos_x_offset(Chunk::BLOCK_SIZE, 0, 0);
    const Vec3f pos_y_offset(0, Chunk::BLOCK_SIZE, 0);
    const Vec3f pos_z_offset(0, 0, Chunk::BLOCK_SIZE);

    for (i32 bx = 0; bx < Chunk::NUM_BLOCKS_PER_AXIS; bx++)
        for (i32 by = 0; by < Chunk::NUM_BLOCKS_PER_AXIS; by++)
            for (i32 bz = 0; bz < Chunk::NUM_BLOCKS_PER_AXIS; bz++)
            {
                // Absolute indexes for the block.
                const i32 abx = cx*Chunk::NUM_BLOCKS_PER_AXIS + bx;
                const i32 aby = cy*Chunk::NUM_BLOCKS_PER_AXIS + by;
                const i32 abz = cz*Chunk::NUM_BLOCKS_PER_AXIS + bz;

                const BlockType block_type = chunk->blocks[bx][by][bz];
                if (block_type == BlockType_Air)
                    continue;

                // Where the block is located in world space.
                const Vec3f block_origin = get_world_coords(*chunk, bx, by, bz);

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

                const bool should_render_bottom_face =
                    (aby == 0) ||
                    !block_exists(abx, aby-1, abz);

                const bool should_render_front_face =
                    (abz == TOTAL_BLOCKS_Z-1) ||
                    !block_exists(abx, aby, abz+1);

                const bool should_render_back_face =
                    (abz == 0) ||
                    !block_exists(abx, aby, abz-1);

                // TODO: Figure out a better way of mixing and matching different
                // block textures.
                u16 sides_layer = -1;
                u16 top_layer = -1;
                u16 bottom_layer = -1;
                if (aby > Landscape::TOTAL_BLOCKS_Y - 30)
                {
                    sides_layer = (should_render_top_face)
                        ? Textures16x16_Snow_Sides_Top
                        : Textures16x16_Snow_Sides;
                    top_layer = Textures16x16_Snow_Top;
                    bottom_layer = Textures16x16_Snow_Bottom;
                }
                else
                {
                    sides_layer = (should_render_top_face)
                        ? Textures16x16_Earth_Sides_Top
                        : Textures16x16_Earth_Sides;
                    top_layer = Textures16x16_Earth_Top;
                    bottom_layer = Textures16x16_Earth_Bottom;
                }

                // Left face ------------------------------------------------------
                if (should_render_left_face)
                {
                    Vertex_PLN v1 = {};
                    v1.position   = left_bottom_back;
                    v1.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v1.normal   = Vec3f(-1, 0, 0);

                    Vertex_PLN v2 = {};
                    v2.position   = left_bottom_front;
                    v2.tex_coords_layer = Vec3f(1, 0, sides_layer);
                    v2.normal   = Vec3f(-1, 0, 0);

                    Vertex_PLN v3 = {};
                    v3.position   = left_top_front;
                    v3.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v3.normal   = Vec3f(-1, 0, 0);

                    Vertex_PLN v4 = {};
                    v4.position   = left_top_front;
                    v4.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v4.normal   = Vec3f(-1, 0, 0);

                    Vertex_PLN v5 = {};
                    v5.position   = left_top_back;
                    v5.tex_coords_layer = Vec3f(0, 1, sides_layer);
                    v5.normal   = Vec3f(-1, 0, 0);

                    Vertex_PLN v6 = {};
                    v6.position   = left_bottom_back;
                    v6.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v6.normal   = Vec3f(-1, 0, 0);

                    chunk_vertices.push_back(v1);
                    chunk_vertices.push_back(v2);
                    chunk_vertices.push_back(v3);
                    chunk_vertices.push_back(v4);
                    chunk_vertices.push_back(v5);
                    chunk_vertices.push_back(v6);
                }
                // Right face -----------------------------------------------------
                if (should_render_right_face)
                {
                    Vertex_PLN v1 = {};
                    v1.position   = right_bottom_back;
                    v1.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v1.normal   = Vec3f(1, 0, 0);

                    Vertex_PLN v2 = {};
                    v2.position   = right_top_back;
                    v2.tex_coords_layer = Vec3f(0, 1, sides_layer);
                    v2.normal   = Vec3f(1, 0, 0);

                    Vertex_PLN v3 = {};
                    v3.position   = right_top_front;
                    v3.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v3.normal     = Vec3f(1, 0, 0);

                    Vertex_PLN v4 = {};
                    v4.position   = right_top_front;
                    v4.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v4.normal     = Vec3f(1, 0, 0);

                    Vertex_PLN v5 = {};
                    v5.position   = right_bottom_front;
                    v5.tex_coords_layer = Vec3f(1, 0, sides_layer);
                    v5.normal   = Vec3f(1, 0, 0);

                    Vertex_PLN v6 = {};
                    v6.position   = right_bottom_back;
                    v6.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v6.normal   = Vec3f(1, 0, 0);

                    chunk_vertices.push_back(v1);
                    chunk_vertices.push_back(v2);
                    chunk_vertices.push_back(v3);
                    chunk_vertices.push_back(v4);
                    chunk_vertices.push_back(v5);
                    chunk_vertices.push_back(v6);
                }
                // Top face --------------------------------------------------------
                if (should_render_top_face)
                {
                    Vertex_PLN v1 = {};
                    v1.position   = left_top_front;
                    v1.tex_coords_layer = Vec3f(0, 0, top_layer);
                    v1.normal   = Vec3f(0, 1, 0);

                    Vertex_PLN v2 = {};
                    v2.position   = right_top_front;
                    v2.tex_coords_layer = Vec3f(1, 0, top_layer);
                    v2.normal   = Vec3f(0, 1, 0);

                    Vertex_PLN v3 = {};
                    v3.position   = right_top_back;
                    v3.tex_coords_layer = Vec3f(1, 1, top_layer);
                    v3.normal   = Vec3f(0, 1, 0);

                    Vertex_PLN v4 = {};
                    v4.position   = right_top_back;
                    v4.tex_coords_layer = Vec3f(1, 1, top_layer);
                    v4.normal   = Vec3f(0, 1, 0);

                    Vertex_PLN v5 = {};
                    v5.position   = left_top_back;
                    v5.tex_coords_layer = Vec3f(0, 1, top_layer);
                    v5.normal   = Vec3f(0, 1, 0);

                    Vertex_PLN v6 = {};
                    v6.position   = left_top_front;
                    v6.tex_coords_layer = Vec3f(0, 0, top_layer);
                    v6.normal   = Vec3f(0, 1, 0);

                    chunk_vertices.push_back(v1);
                    chunk_vertices.push_back(v2);
                    chunk_vertices.push_back(v3);
                    chunk_vertices.push_back(v4);
                    chunk_vertices.push_back(v5);
                    chunk_vertices.push_back(v6);
                }
                // Bottom face -----------------------------------------------------
                if (should_render_bottom_face)
                {
                    Vertex_PLN v1 = {};
                    v1.position   = left_bottom_back;
                    v1.tex_coords_layer = Vec3f(0, 0, bottom_layer);
                    v1.normal   = Vec3f(0, -1, 0);

                    Vertex_PLN v2 = {};
                    v2.position   = right_bottom_back;
                    v2.tex_coords_layer = Vec3f(1, 0, bottom_layer);
                    v2.normal   = Vec3f(0, -1, 0);

                    Vertex_PLN v3 = {};
                    v3.position   = right_bottom_front;
                    v3.tex_coords_layer = Vec3f(1, 1, bottom_layer);
                    v3.normal   = Vec3f(0, -1, 0);

                    Vertex_PLN v4 = {};
                    v4.position   = right_bottom_front;
                    v4.tex_coords_layer = Vec3f(1, 1, bottom_layer);
                    v4.normal   = Vec3f(0, -1, 0);

                    Vertex_PLN v5 = {};
                    v5.position   = left_bottom_front;
                    v5.tex_coords_layer = Vec3f(0, 1, bottom_layer);
                    v5.normal   = Vec3f(0, -1, 0);

                    Vertex_PLN v6 = {};
                    v6.position   = left_bottom_back;
                    v6.tex_coords_layer = Vec3f(0, 0, bottom_layer);
                    v6.normal   = Vec3f(0, -1, 0);

                    chunk_vertices.push_back(v1);
                    chunk_vertices.push_back(v2);
                    chunk_vertices.push_back(v3);
                    chunk_vertices.push_back(v4);
                    chunk_vertices.push_back(v5);
                    chunk_vertices.push_back(v6);
                }
                // Front face -------------------------------------------------------
                if (should_render_front_face)
                {
                    Vertex_PLN v1 = {};
                    v1.position   = left_bottom_front;
                    v1.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v1.normal   = Vec3f(0, 0, 1);

                    Vertex_PLN v2 = {};
                    v2.position   = right_bottom_front;
                    v2.tex_coords_layer = Vec3f(1, 0, sides_layer);
                    v2.normal   = Vec3f(0, 0, 1);

                    Vertex_PLN v3 = {};
                    v3.position   = right_top_front;
                    v3.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v3.normal   = Vec3f(0, 0, 1);

                    Vertex_PLN v4 = {};
                    v4.position   = right_top_front;
                    v4.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v4.normal   = Vec3f(0, 0, 1);

                    Vertex_PLN v5 = {};
                    v5.position   = left_top_front;
                    v5.tex_coords_layer = Vec3f(0, 1, sides_layer);
                    v5.normal   = Vec3f(0, 0, 1);

                    Vertex_PLN v6 = {};
                    v6.position   = left_bottom_front;
                    v6.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v6.normal   = Vec3f(0, 0, 1);

                    chunk_vertices.push_back(v1);
                    chunk_vertices.push_back(v2);
                    chunk_vertices.push_back(v3);
                    chunk_vertices.push_back(v4);
                    chunk_vertices.push_back(v5);
                    chunk_vertices.push_back(v6);
                }
                // Back face --------------------------------------------------------
                if (should_render_back_face)
                {
                    Vertex_PLN v1 = {};
                    v1.position   = right_bottom_back;
                    v1.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v1.normal   = Vec3f(0, 0, -1);

                    Vertex_PLN v2 = {};
                    v2.position   = left_bottom_back;
                    v2.tex_coords_layer = Vec3f(1, 0, sides_layer);
                    v2.normal   = Vec3f(0, 0, -1);

                    Vertex_PLN v3 = {};
                    v3.position   = left_top_back;
                    v3.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v3.normal   = Vec3f(0, 0, -1);

                    Vertex_PLN v4 = {};
                    v4.position   = left_top_back;
                    v4.tex_coords_layer = Vec3f(1, 1, sides_layer);
                    v4.normal   = Vec3f(0, 0, -1);

                    Vertex_PLN v5 = {};
                    v5.position   = right_top_back;
                    v5.tex_coords_layer = Vec3f(0, 1, sides_layer);
                    v5.normal   = Vec3f(0, 0, -1);

                    Vertex_PLN v6 = {};
                    v6.position   = right_bottom_back;
                    v6.tex_coords_layer = Vec3f(0, 0, sides_layer);
                    v6.normal   = Vec3f(0, 0, -1);

                    chunk_vertices.push_back(v1);
                    chunk_vertices.push_back(v2);
                    chunk_vertices.push_back(v3);
                    chunk_vertices.push_back(v4);
                    chunk_vertices.push_back(v5);
                    chunk_vertices.push_back(v6);
                }
            }
    return chunk_vertices;
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
    f64 map[Landscape::Chunk::NUM_BLOCKS_PER_AXIS][Landscape::Chunk::NUM_BLOCKS_PER_AXIS];
};

ChunkNoise *
Landscape::fill_noise_map_for_chunk_column(f32 origin_x, f32 origin_z)
{
    //
    // PERFORMANCE: remove heap allocated buffer for better performance.
    //
    auto chunk_noise = new ChunkNoise();
    LT_Assert(chunk_noise);

    // Fill out noise_map
    for (i32 z = 0; z < Chunk::NUM_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < Chunk::NUM_BLOCKS_PER_AXIS; x++)
        {
            const f64 noise_x = (f64)origin_x + Chunk::BLOCK_SIZE*x;
            const f64 noise_y = (f64)origin_z + Chunk::BLOCK_SIZE*z;
            const f64 noise = get_fbm(m_simplex_ctx, noise_x, noise_y, m_amplitude,
                                      m_frequency, m_num_octaves, m_lacunarity, m_gain);
            chunk_noise->map[z][x] = noise;
        }

    // Rescale noise values into 0 -> 1 range
    const f64 min = -1.0;
    const f64 max = 1.0;

    // // - Actually rescale the values
    for (i32 z = 0; z < Chunk::NUM_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < Chunk::NUM_BLOCKS_PER_AXIS; x++)
        {
            chunk_noise->map[z][x] = (chunk_noise->map[z][x] + (max - min)) / (2*max - min);

#define EPSILON 0.001
            LT_Assert(chunk_noise->map[z][x] <= (max+EPSILON) && chunk_noise->map[z][x] >= (min-EPSILON));

            LT_Assert(chunk_noise->map[z][x] <= (1.0 + EPSILON) && chunk_noise->map[z][x] >= -EPSILON);
#undef EPSILON
        }

    return chunk_noise;
}

void
Landscape::pass_chunk_buffer_to_gpu(const VAOArray::Entry &entry, const std::vector<Vertex_PLN> &buf)
{
    glBindVertexArray(entry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, entry.vbo);
    // TODO: Figure out if GL_DYNAMIC_DRAW is the best enum to use or there's something better.
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_PLN) * entry.num_vertices_used, &buf[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                          (const void*)offsetof(Vertex_PLN, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                          (const void*)offsetof(Vertex_PLN, tex_coords_layer));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PLN),
                          (const void*)offsetof(Vertex_PLN, normal));
    glEnableVertexAttribArray(2);

    glDrawArrays(GL_TRIANGLES, 0, entry.num_vertices_used);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
Landscape::generate()
{
    logger.log("Generating landscape");

    for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
        for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
        {
            chunks_mutex.lock_high_priority(); // LOCK

            Chunk *chunk = chunk_ptrs[cx][0][cz].get();
            LT_Assert(chunk);
            ChunkNoise *chunk_noise = fill_noise_map_for_chunk_column(chunk->origin.x, chunk->origin.z);

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

            chunks_mutex.unlock_high_priority(); // UNLOCK
            delete[] chunk_noise;
        }

    for (i32 cx = 0; cx < NUM_CHUNKS_X; cx++)
        for (i32 cy = 0; cy < NUM_CHUNKS_Y; cy++)
            for (i32 cz = 0; cz < NUM_CHUNKS_Z; cz++)
            {
                chunks_mutex.lock_high_priority(); // LOCK

                Chunk *chunk = chunk_ptrs[cx][cy][cz].get();
                chunk->create_request();
                m_chunks_to_process_queues[QP_Low].insert(chunk->request, &m_chunks_to_process_semaphore);

                chunks_mutex.unlock_high_priority(); // UNLOCK
            }
}

void
Landscape::do_chunk_generation_work(Chunk *chunk)
{
    const i32 cy = static_cast<i32>(chunk->origin.y - origin.y) / Chunk::SIZE;
    ChunkNoise *chunk_noise = fill_noise_map_for_chunk_column(chunk->origin.x, chunk->origin.z);
    LT_Assert(chunk_noise);

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
                    chunk->blocks[bx][by][bz] = BlockType_Terrain;
            }
            else if (height_cy == cy)
            {
                for (i32 by = 0; by <= height_by; by++)
                    chunk->blocks[bx][by][bz] = BlockType_Terrain;
            }
        }

    delete[] chunk_noise;
}

void
Landscape::run_worker_thread()
{
    logger.log("Thread ", std::this_thread::get_id(), " started");
    while (m_threads_should_run)
    {
        for (i32 i = 0; i < QP_Count; i++)
        {
            auto &queue = m_chunks_to_process_queues[i];
            auto request = queue.take_next_request();

            if (request) // there is a request to process.
            {
                chunks_mutex.lock_low_priority(); // LOCK

                if (request->chunk)
                {
                    request->vertexes = update_chunk_buffer(request->chunk);
                    request->processed = true;
                    m_chunks_processed_queues[i].insert(request, nullptr);
                }
                else
                {
                    // NOTE: If chunk is null, it means the request was cancelled, so it should be ignored.
                }

                chunks_mutex.unlock_low_priority(); // UNLOCK
                break;
            }
        }

        // Wait for more work to be added to the queue.
        m_chunks_to_process_semaphore.wait();
    }
}

void
Landscape::stop_threads()
{
    logger.log("Stopping threads...");
    // Notify all threads that they should not run anymore.
    m_threads_should_run = false;
    m_chunks_to_process_semaphore.notify_all(m_threads.size());
    for (auto &thread : m_threads) thread.join();
}

void
Landscape::initialize_threads()
{
    logger.log("Initializing threads...");
    m_threads_should_run = true;
    for (auto &thread : m_threads)
    {
        thread = std::thread(std::bind(&Landscape::run_worker_thread, _1), this);
    }
}

void
Landscape::chunk_deleter(Chunk *chunk)
{
    memory::destroy_and_deallocate(m_chunks_allocator, chunk);
}


// ----------------------------------------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------------------------------------

Landscape::Chunk::Chunk(Vec3f origin, VAOArray *va, BlockType fill_type)
    : origin(origin)
    , request(nullptr)
    , m_vao_array(va)
{
    entry_index = m_vao_array->take_free_entry();

    for (i32 x = 0; x < NUM_BLOCKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_BLOCKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_BLOCKS_PER_AXIS; z++)
                blocks[x][y][z] = fill_type;
}

Landscape::Chunk::~Chunk()
{
    m_vao_array->free_entry(entry_index);
}

void
Landscape::Chunk::create_request()
{
    // TODO: maybe remove using the heap for the allocation of this object.
    request = std::make_shared<QueueRequest>(this);
}

void
Landscape::Chunk::cancel_request()
{
    if (request && request->chunk)
        request->chunk = nullptr;
}

// ----------------------------------------------------------------------------------------------
// VBOArray, Chunk Queue and Queue Entry
// ----------------------------------------------------------------------------------------------
Landscape::VAOArray::Entry::Entry()
    : vao(GLResources::instance().create_vertex_array())
    , vbo(GLResources::instance().create_buffer())
    , num_vertices_used(0)
    , is_used(false)
{}

isize
Landscape::VAOArray::take_free_entry()
{
    for (isize i = 0; i < NUM_CHUNKS; i++)
    {
        if (!vaos[i].is_used)
        {
            vaos[i].is_used = true;
            vaos[i].num_vertices_used = 0;
            return i;
        }
    }
    LT_Panic("There is not enough free indices.");
}

void
Landscape::VAOArray::free_entry(isize index)
{
    LT_Assert(index >= 0);
    LT_Assert(index < NUM_CHUNKS);
    LT_Assert(vaos[index].is_used);

    vaos[index].is_used = false;
}

void
Landscape::ChunkQueue::insert(const std::shared_ptr<QueueRequest> &request, Semaphore *semaphore)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    const i32 next_write_index = (write_index + 1) % MAX_ENTRIES;
    // if next write and read index are the same, the queue is full.
    LT_Assert(next_write_index != read_index);

    requests[write_index] = request;
    write_index = next_write_index;
    if (semaphore) semaphore->notify();
}

std::shared_ptr<Landscape::QueueRequest>
Landscape::ChunkQueue::take_next_request()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (read_index != write_index) // There is an entry to consume
    {
        const i32 next_read_index = (read_index + 1) % MAX_ENTRIES;
        std::shared_ptr<QueueRequest> req = requests[read_index];
        read_index = next_read_index;
        return req;
    }
    else // There is not an entry to consume, return a null pointer
    {
        return nullptr;
    }
}

void
Landscape::remove_block(Vec3f ray_origin, Vec3f ray_direction)
{
    // NOTE: This function implements the Fast Voxel Traversal algorithm with some changes added to
    // work with negative directions.
    // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.3443&rep=rep1&type=pdf

    chunks_mutex.lock_high_priority(); // LOCK
    // Find the block where the ray starts.
    const Vec3f offset_from_origin = ray_origin - origin;
    LT_Assert(offset_from_origin.x >= 0 && offset_from_origin.y >= 0 && offset_from_origin.z >= 0);

    i32 abx = std::floor(offset_from_origin.x / Chunk::BLOCK_SIZE);
    i32 aby = std::floor(offset_from_origin.y / Chunk::BLOCK_SIZE);
    i32 abz = std::floor(offset_from_origin.z / Chunk::BLOCK_SIZE);

    const i32 step_x = lt::sign_float(ray_direction.x);
    const i32 step_y = lt::sign_float(ray_direction.y);
    const i32 step_z = lt::sign_float(ray_direction.z);

    // How much we can travel with t until we encounter a voxel boundary in each axis.
    const f32 t_delta_x = (step_x != 0)
        ? (f32)Chunk::BLOCK_SIZE / ray_direction.x
        : std::numeric_limits<f32>::max();

    const f32 t_delta_y = (step_y != 0)
        ? (f32)Chunk::BLOCK_SIZE / ray_direction.y
        : std::numeric_limits<f32>::max();

    const f32 t_delta_z = (step_z != 0)
        ? (f32)Chunk::BLOCK_SIZE / ray_direction.z
        : std::numeric_limits<f32>::max();

    auto fraction_pos = [](f32 x) -> f32 { // fraction when direction is positive
        return (x - std::floor(x));
    };

    // Maximum amount we can advance (in t) in order to meet one of the axis boundaries.
    // f32 t_max_x = t_delta_x * (1 - fraction_pos(offset_from_origin.x / Chunk::BLOCK_SIZE));
    f32 t_max_x = (step_x > 0)
        ? t_delta_x * (1 - fraction_pos(offset_from_origin.x / Chunk::BLOCK_SIZE))
        : t_delta_x * fraction_pos(offset_from_origin.x / Chunk::BLOCK_SIZE);

    f32 t_max_y = (step_y > 0)
        ? t_delta_y * (1 - fraction_pos(offset_from_origin.y / Chunk::BLOCK_SIZE))
        : t_delta_y * fraction_pos(offset_from_origin.y / Chunk::BLOCK_SIZE);

    f32 t_max_z = (step_z > 0)
        ? t_delta_z * (1 - fraction_pos(offset_from_origin.z / Chunk::BLOCK_SIZE))
        : t_delta_z * fraction_pos(offset_from_origin.z / Chunk::BLOCK_SIZE);

    i32 blocks_traversed = 0;
    for (;;) {
        if (std::abs(t_max_x) < std::abs(t_max_y)) {
            if (std::abs(t_max_x) < std::abs(t_max_z)) {
                abx += step_x;
                t_max_x += t_delta_x;
            } else {
                abz += step_z;
                t_max_z += t_delta_z;
            }
        } else {
            if (std::abs(t_max_y) < std::abs(t_max_z)) {
                aby += step_y;
                t_max_y += t_delta_y;
            } else {
                abz += step_z;
                t_max_z += t_delta_z;
            }
        }

        const i32 bx = abx % Chunk::NUM_BLOCKS_PER_AXIS;
        const i32 by = aby % Chunk::NUM_BLOCKS_PER_AXIS;
        const i32 bz = abz % Chunk::NUM_BLOCKS_PER_AXIS;

        const i32 cx = abx / Chunk::NUM_BLOCKS_PER_AXIS;
        const i32 cy = aby / Chunk::NUM_BLOCKS_PER_AXIS;
        const i32 cz = abz / Chunk::NUM_BLOCKS_PER_AXIS;

        Chunk *chunk = chunk_ptrs[cx][cy][cz].get();
        if (chunk->blocks[bx][by][bz] != BlockType_Air)
        {
            chunk->blocks[bx][by][bz] = BlockType_Air;
            chunk->create_request();

            // Regenerate the current chunk mesh.
            m_chunks_to_process_queues[QP_High].insert(chunk->request, &m_chunks_to_process_semaphore);

            // If the block changed is in the boundary of another chunk,
            // update the neighboring chunk as well.
            if (bx == 0)
            {
                Chunk *left_chunk = chunk_ptrs[cx-1][cy][cz].get();
                left_chunk->create_request();
                m_chunks_to_process_queues[QP_High].insert(left_chunk->request,
                                                          &m_chunks_to_process_semaphore);
            }
            if (bx == Chunk::NUM_BLOCKS_PER_AXIS-1)
            {
                Chunk *right_chunk = chunk_ptrs[cx+1][cy][cz].get();
                right_chunk->create_request();
                m_chunks_to_process_queues[QP_High].insert(right_chunk->request,
                                                          &m_chunks_to_process_semaphore);
            }
            if (by == 0)
            {
                Chunk *bottom_chunk = chunk_ptrs[cx][cy-1][cz].get();
                bottom_chunk->create_request();
                m_chunks_to_process_queues[QP_High].insert(bottom_chunk->request,
                                                          &m_chunks_to_process_semaphore);
            }
            if (by == Chunk::NUM_BLOCKS_PER_AXIS-1)
            {
                Chunk *top_chunk = chunk_ptrs[cx][cy+1][cz].get();
                top_chunk->create_request();
                m_chunks_to_process_queues[QP_High].insert(top_chunk->request,
                                                          &m_chunks_to_process_semaphore);
            }
            if (bz == 0)
            {
                Chunk *back_chunk = chunk_ptrs[cx][cy][cz-1].get();
                back_chunk->create_request();
                m_chunks_to_process_queues[QP_High].insert(back_chunk->request,
                                                          &m_chunks_to_process_semaphore);
            }
            if (bz == Chunk::NUM_BLOCKS_PER_AXIS-1)
            {
                Chunk *front_chunk = chunk_ptrs[cx][cy][cz+1].get();
                front_chunk->create_request();
                m_chunks_to_process_queues[QP_High].insert(front_chunk->request,
                                                          &m_chunks_to_process_semaphore);
            }
            break;
        }

        blocks_traversed++;
        if (blocks_traversed == 6) break; // TODO: remove hardcoded number of blocks.
    }

    chunks_mutex.unlock_high_priority(); // UNLOCK
}
