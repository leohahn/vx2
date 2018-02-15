#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"
#include "skybox.hpp"
#include "camera.hpp"

struct Key;
struct osn_context;

struct Sun
{
    Vec3f direction;
    Vec3f ambient;
    Vec3f diffuse;
    Vec3f specular;
};

enum BlockType
{
    BlockType_Air = 0,
    BlockType_Terrain = 1,
};

struct Chunk
{
    constexpr static i32 NUM_BLOCKS_PER_AXIS = 16;
    constexpr static i32 NUM_BLOCKS = NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS;
    constexpr static i32 BLOCK_SIZE = 1;

    BlockType blocks[NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS];
    Vec3f origin;
    u32 vao, vbo;
};

enum WorldState
{
    WorldState_InitialLoad,
    WorldState_Running,
    WorldState_Paused,
};

struct World
{
    constexpr static i32 NUM_CHUNKS_PER_AXIS = 1;
    constexpr static i32 TOTAL_BLOCKS_PER_AXIS = NUM_CHUNKS_PER_AXIS * Chunk::NUM_BLOCKS_PER_AXIS;

    World(const ResourceManager &manager, f32 aspect_ratio);
    ~World();

    Camera camera;
    Chunk chunks[NUM_CHUNKS_PER_AXIS][NUM_CHUNKS_PER_AXIS][NUM_CHUNKS_PER_AXIS];
    Skybox skybox;
    Sun sun;
    Vec3f origin;

    void update(Key *kb);
    void generate_landscape(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);

    WorldState state = WorldState_InitialLoad;
    bool render_wireframe = false;

private:
    osn_context *m_simplex_ctx;
    void initialize_buffers();
    Camera create_camera(f32 aspect_ratio);
};

#endif // __WORLD_HPP__
