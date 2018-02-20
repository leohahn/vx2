#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"
#include "skybox.hpp"
#include "camera.hpp"

struct Key;
struct osn_context;
struct GLResources;
struct TextureAtlas;

struct Sun
{
    Vec3f direction;
    Vec3f ambient;
    Vec3f diffuse;
    Vec3f specular;
};

enum BlockType
{
    BlockType_Terrain = 0,
    BlockType_Count,
    BlockType_Air = -1, // it is -1 because it is not textured
};

struct BlocksTextureInfo
{
    enum Layer
    {
        Sides_Top_Covered = 0,
        Sides_Top_Uncovered = 1,
        Top = 2,
        Bottom = 3,
    };

    BlocksTextureInfo(const char *texture_name, const ResourceManager &manager);
    bool load();
    u32 texture_id() const;

private:
    TextureAtlas *m_texture;
};

struct Chunk
{
    constexpr static i32 NUM_BLOCKS_PER_AXIS = 16;
    constexpr static i32 NUM_BLOCKS = NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS;
    constexpr static i32 BLOCK_SIZE = 1;

    Chunk()
        : vao(0), vbo(0), outdated(true)
    {
        for (i32 x = 0; x < NUM_BLOCKS_PER_AXIS; x++)
            for (i32 y = 0; y < NUM_BLOCKS_PER_AXIS; y++)
                for (i32 z = 0; z < NUM_BLOCKS_PER_AXIS; z++)
                    blocks[x][y][z] = BlockType_Air;
    }

    Chunk &operator=(const Chunk &chunk)
    {
        vao = chunk.vao;
        vbo = chunk.vbo;
        num_vertices_used = chunk.num_vertices_used;
        origin = chunk.origin;
        outdated = chunk.outdated;
        std::memcpy(blocks, chunk.blocks, sizeof(BlockType)*pow(Chunk::NUM_BLOCKS_PER_AXIS, 3));
        std::memcpy(max_vertices, chunk.max_vertices, sizeof(Vec3f)*LT_Count(max_vertices));

        return *this;
    }

    BlockType blocks[NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS];
    Vec3f     max_vertices[8];
    Vec3f     origin;
    u32       vao, vbo;
    // Signals that the chunk's buffer must be updated.
    bool      outdated;
    i32       num_vertices_used;
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
    constexpr static i32 NUM_CHUNKS_X = 6;
    constexpr static i32 NUM_CHUNKS_Y = 3;
    constexpr static i32 NUM_CHUNKS_Z = 6;
    constexpr static i32 TOTAL_BLOCKS_X = NUM_CHUNKS_X * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Y = NUM_CHUNKS_Y * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Z = NUM_CHUNKS_Z * Chunk::NUM_BLOCKS_PER_AXIS;

    static World interpolate(const World &pevious, const World &current, f32 alpha);

    World(const World &world);
    // World(World &&world);
    World &operator=(const World &world);

    World(i32 seed, const char *blocks_texture, const ResourceManager &manager,
          GLResources *gl_resources, f32 aspect_ratio);
    ~World();

    void update(Key *kb);
    void generate_landscape(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);
    bool block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi) const;

    void update_chunk_buffer(i32 cx, i32 cy, i32 cz);

public:
    Camera        camera;
    Chunk         chunks[NUM_CHUNKS_X][NUM_CHUNKS_Y][NUM_CHUNKS_Z];
    Skybox        skybox;
    BlocksTextureInfo blocks_texture_info;
    Sun           sun;
    Vec3f         origin;
    WorldStatus   state = WorldStatus_InitialLoad;
    bool          render_wireframe = false;

private:
    i32           m_seed;
    osn_context  *m_simplex_ctx;
    GLResources  *m_gl;

    void initialize_buffers();
    Camera create_camera(f32 aspect_ratio);
};

#endif // __WORLD_HPP__
