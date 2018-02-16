#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"
#include "skybox.hpp"
#include "camera.hpp"

struct Key;
struct osn_context;
struct GLResources;

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

enum WorldStatus
{
    WorldStatus_InitialLoad,
    WorldStatus_Running,
    WorldStatus_Paused,
};

struct WorldState
{
    Frustum camera_frustum;
};

struct World
{
    constexpr static i32 NUM_CHUNKS_PER_AXIS = 1;
    constexpr static i32 TOTAL_BLOCKS_PER_AXIS = NUM_CHUNKS_PER_AXIS * Chunk::NUM_BLOCKS_PER_AXIS;

    static World interpolate(const World &pevious, const World &current, f32 alpha);

    World(const World &world);
    // World(World &&world);
    World &operator=(const World &world);

    World(i32 seed, const ResourceManager &manager, GLResources *gl_resources, f32 aspect_ratio);
    ~World();

    void update(Key *kb);
    void generate_landscape(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);
    bool block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi) const;

public:
    Camera      camera;
    Chunk       chunks[NUM_CHUNKS_PER_AXIS][NUM_CHUNKS_PER_AXIS][NUM_CHUNKS_PER_AXIS];
    Skybox      skybox;
    Sun         sun;
    Vec3f       origin;
    WorldStatus state = WorldStatus_InitialLoad;
    bool        render_wireframe = false;

private:
    i32         m_seed;
    osn_context *m_simplex_ctx;
    GLResources *m_gl;

    void initialize_buffers();
    Camera create_camera(f32 aspect_ratio);
};

#endif // __WORLD_HPP__
