#include "world.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "resource_manager.hpp"
#include "lt_utils.hpp"
#include "input.hpp"
#include "open-simplex-noise.h"
#include "gl_resources.hpp"
#include "texture.hpp"

lt_global_variable lt::Logger logger("world");

void
World::initialize_buffers()
{
    logger.log("Initialize buffers called.");
    for (i32 x = 0; x < NUM_CHUNKS_X; x++)
        for (i32 y = 0; y < NUM_CHUNKS_Y; y++)
            for (i32 z = 0; z < NUM_CHUNKS_Z; z++)
            {
                const f32 offset = Chunk::BLOCK_SIZE * Chunk::NUM_BLOCKS_PER_AXIS;
                const Vec3f chunk_origin = origin + Vec3f(x*offset, y*offset, z*offset);
                chunks[x][y][z].origin = chunk_origin;
                chunks[x][y][z].vao = m_gl->create_vertex_array();
                chunks[x][y][z].vbo = m_gl->create_buffer();
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

Camera
World::create_camera(f32 aspect_ratio)
{
    const f32 FIELD_OF_VIEW = 60.0f;
    const f32 MOVE_SPEED = 0.13f;
    const f32 ROTATION_SPEED = 0.033f;
    const Vec3f CAMERA_POSITION(15, 20, 45);
    const Vec3f CAMERA_FRONT(0, 0, -1);
    const Vec3f UP_WORLD(0.0f, 1.0f, 0.0f);
    return Camera(CAMERA_POSITION, CAMERA_FRONT, UP_WORLD,
                  FIELD_OF_VIEW, aspect_ratio, MOVE_SPEED, ROTATION_SPEED);
}

World::World(i32 seed, const char *blocks_texture, const ResourceManager &manager,
             GLResources *gl, f32 aspect_ratio)
    : camera(create_camera(aspect_ratio))
    , chunks()
    , skybox("skybox.texture", "skybox.shader", manager)
    , blocks_texture_info(blocks_texture, manager)
    , sun()
    , m_seed(seed)
    , m_gl(gl)
{
    initialize_buffers();

    sun.direction = lt::normalize(Vec3f(0.49f, -0.35f, 0.80f));
    sun.ambient = Vec3f(.1f);
    sun.diffuse = Vec3f(.7f);
    sun.specular = Vec3f(1.0f);

    if (open_simplex_noise(seed, &m_simplex_ctx))
    {
        logger.error("Failed to initialize context for noise generation.");
        return;
    }
}

World::World(const World &world)
    : blocks_texture_info(world.blocks_texture_info)
{
    *this = world;

    if (open_simplex_noise(world.m_seed, &m_simplex_ctx))
        logger.error("Failed to initialize context for noise generation.");

    for (i32 x = 0; x < NUM_CHUNKS_X; x++)
        for (i32 y = 0; y < NUM_CHUNKS_Y; y++)
            for (i32 z = 0; z < NUM_CHUNKS_Z; z++)
            {
                chunks[x][y][z].origin = world.chunks[x][y][z].origin;
                chunks[x][y][z].vao = world.chunks[x][y][z].vao;
                chunks[x][y][z].vbo = world.chunks[x][y][z].vbo;
                for (i32 i = 0; i < 8; i++)
                    chunks[x][y][z].max_vertices[i] = world.chunks[x][y][z].max_vertices[i];

                m_gl->add_reference_to_vertex_array(chunks[x][y][z].vao);
                m_gl->add_reference_to_buffer(chunks[x][y][z].vbo);
            }
}

World &
World::operator=(const World &world)
{
    camera = world.camera;
    skybox = world.skybox;
    blocks_texture_info = world.blocks_texture_info;
    sun = world.sun;
    origin = world.origin;
    state = world.state;
    render_wireframe = world.render_wireframe;
    m_seed = world.m_seed;
    m_gl = world.m_gl;

    for (i32 x = 0; x < NUM_CHUNKS_X; x++)
        for (i32 y = 0; y < NUM_CHUNKS_Y; y++)
            for (i32 z = 0; z < NUM_CHUNKS_Z; z++)
            {
                chunks[x][y][z].origin = world.chunks[x][y][z].origin;
                std::memcpy(chunks[x][y][z].blocks, world.chunks[x][y][z].blocks,
                            sizeof(BlockType)*pow(Chunk::NUM_BLOCKS_PER_AXIS, 3));
            }

    return *this;
}

World::~World()
{
    for (i32 x = 0; x < NUM_CHUNKS_X; x++)
        for (i32 y = 0; y < NUM_CHUNKS_Y; y++)
            for (i32 z = 0; z < NUM_CHUNKS_Z; z++)
            {
                m_gl->delete_vertex_array(chunks[x][y][z].vao);
                m_gl->delete_buffer(chunks[x][y][z].vbo);
            }

    open_simplex_noise_free(m_simplex_ctx);
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
World::generate_landscape(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain)
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

bool
World::block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi) const
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
World::update(Key *kb)
{
    if (skybox.load() && blocks_texture_info.load())
    {
        state = WorldStatus_Running;
    }

    camera.update(kb);

    if (kb[GLFW_KEY_T].last_transition == Key::Transition_Down)
        render_wireframe = !render_wireframe;
}

World
World::interpolate(const World &previous, const World &current, f32 alpha)
{
    LT_Assert(previous.camera.up_world == current.camera.up_world);

    World interpolated = current;
    interpolated.camera = Camera::interpolate(previous.camera, current.camera, alpha);

    return interpolated;
}

BlocksTextureInfo::BlocksTextureInfo(const char *texture_name, const ResourceManager &manager)
    : m_texture(manager.get_texture<TextureAtlas>(texture_name))
{
    if (!m_texture)
        logger.error("Failed to get texture ", texture_name);
}

bool BlocksTextureInfo::load() { return m_texture->load(); }

u32 BlocksTextureInfo::texture_id() const { return m_texture->id; }
