#include "world.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "resource_manager.hpp"
#include "lt_utils.hpp"
#include "input.hpp"
#include "open-simplex-noise.h"

lt_global_variable lt::Logger logger("world");

void
World::initialize_buffers()
{
    for (i32 x = 0; x < NUM_CHUNKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_CHUNKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_CHUNKS_PER_AXIS; z++)
            {
                glGenVertexArrays(1, &chunks[x][y][z].vao);
                glGenBuffers(1, &chunks[x][y][z].vbo);
            }
}

Camera
World::create_camera(f32 aspect_ratio)
{
    const f32 FIELD_OF_VIEW = 60.0f;
    const f32 MOVE_SPEED = 0.13f;
    const f32 ROTATION_SPEED = 0.033f;
    const Vec3f CAMERA_POSITION(0, 0, 15);
    const Vec3f CAMERA_FRONT(0, 0, -1);
    const Vec3f UP_WORLD(0.0f, 1.0f, 0.0f);
    return Camera(CAMERA_POSITION, CAMERA_FRONT, UP_WORLD,
                  FIELD_OF_VIEW, aspect_ratio, MOVE_SPEED, ROTATION_SPEED);
}

World::World(const ResourceManager &manager, f32 aspect_ratio)
    : camera(create_camera(aspect_ratio))
    , chunks()
    , skybox("skybox.texture", "skybox.glsl", manager)
    , sun()
{
    initialize_buffers();

    sun.direction = Vec3f(0, -1, 0);
    sun.ambient = Vec3f(.1f);
    sun.diffuse = Vec3f(.7f);
    sun.specular = Vec3f(1.0f);

    const i32 seed = 1024;
    if (open_simplex_noise(seed, &m_simplex_ctx))
    {
        logger.error("Failed to initialize context for noise generation.");
        return;
    }
}

World::~World()
{
    for (i32 x = 0; x < NUM_CHUNKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_CHUNKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_CHUNKS_PER_AXIS; z++)
                glDeleteBuffers(1, &chunks[x][y][z].vbo);

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
    logger.log("Total number of blocks per axis: ", (i32)World::TOTAL_BLOCKS_PER_AXIS);

    f64 noise_map[TOTAL_BLOCKS_PER_AXIS*TOTAL_BLOCKS_PER_AXIS] = {};

    // Fill out noise_map
    logger.log("Generating noise map");
    for (i32 z = 0; z < TOTAL_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < TOTAL_BLOCKS_PER_AXIS; x++)
        {
            const f64 noise_x = (f64)x;
            const f64 noise_y = (f64)z;
            f64 noise = get_fbm(m_simplex_ctx, noise_x, noise_y, amplitude,
                                frequency, num_octaves, lacunarity, gain);
            noise_map[x + z*TOTAL_BLOCKS_PER_AXIS] = noise;
        }


    logger.log("Rescaling noise map");
    //
    // Rescale noise values into 0 -> 1 range
    //
    // - Find first the maximum and minimum values
    f64 min = std::numeric_limits<f64>::max();
    f64 max = std::numeric_limits<f64>::min();
    for (i32 z = 0; z < TOTAL_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < TOTAL_BLOCKS_PER_AXIS; x++)
        {
            const i32 i = x + z*TOTAL_BLOCKS_PER_AXIS;
            if (noise_map[i] > max)
                max = noise_map[i];
            if (noise_map[i] < min)
                min = noise_map[i];
        }

    // - Actually rescale the values
    for (i32 z = 0; z < TOTAL_BLOCKS_PER_AXIS; z++)
        for (i32 x = 0; x < TOTAL_BLOCKS_PER_AXIS; x++)
        {
            const i32 i = x + z*TOTAL_BLOCKS_PER_AXIS;

            f64 noise = noise_map[i];
            noise_map[i] = (noise + (max - min)) / (2*max - min);

            const f64 EPSILON = 0.002;
            LT_Assert(noise_map[i] <= (1.0 + EPSILON) && noise_map[i] >= -EPSILON);
        }

    logger.log("Filling out blocks with noise map");
    for (i32 abs_block_x = 0; abs_block_x < TOTAL_BLOCKS_PER_AXIS; abs_block_x++)
        for (i32 abs_block_z = 0; abs_block_z < TOTAL_BLOCKS_PER_AXIS; abs_block_z++)
        {
            const i32 block_xi = abs_block_x % Chunk::NUM_BLOCKS_PER_AXIS;
            const i32 block_zi = abs_block_z % Chunk::NUM_BLOCKS_PER_AXIS;

            const i32 chunk_xi = abs_block_x / Chunk::NUM_BLOCKS_PER_AXIS;
            const i32 chunk_zi = abs_block_z / Chunk::NUM_BLOCKS_PER_AXIS;

            const i32 noise_index = abs_block_x + abs_block_z*TOTAL_BLOCKS_PER_AXIS;
            const f64 height = noise_map[noise_index];

            const i32 abs_block_y = std::round((f64)(TOTAL_BLOCKS_PER_AXIS-1) * height);

            logger.log("for chunk_xi = ", chunk_xi);
            logger.log("for chunk_zi = ", chunk_zi);
            logger.log("for block_xi = ", block_xi);
            logger.log("for block_zi = ", block_zi);
            logger.log("height = ", height);

            for (i32 y = 0; y <= abs_block_y; y++)
            {
                const i32 block_yi = y % Chunk::NUM_BLOCKS_PER_AXIS;
                const i32 chunk_yi = y / Chunk::NUM_BLOCKS_PER_AXIS;

                Chunk &chunk = chunks[chunk_xi][chunk_yi][chunk_zi];
                chunk.blocks[block_xi][block_yi][block_zi] = BlockType_Terrain;
            }
        }
}

void
World::update(Key *kb)
{
    if (skybox.load())
    {
        state = WorldState_Running;
    }

    camera.update(kb);

    if (kb[GLFW_KEY_T].last_transition == Key::Transition_Down)
        render_wireframe = !render_wireframe;
}
