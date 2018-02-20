#ifndef __LANDSCAPE_HPP__
#define __LANDSCAPE_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"

struct TextureAtlas;
struct ResourceManager;
struct osn_context;

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

    Chunk();
    ~Chunk();
    Chunk(Chunk &chunk) = delete;
    Chunk &operator=(const Chunk &chunk) = delete;

    BlockType blocks[NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS];
    Vec3f     max_vertices[8];
    Vec3f     origin;
    u32       vao, vbo;
    // Signals that the chunk's buffer must be updated.
    bool      outdated;
    i32       num_vertices_used;
};

struct Landscape
{
    constexpr static i32 NUM_CHUNKS_X = 6;
    constexpr static i32 NUM_CHUNKS_Y = 3;
    constexpr static i32 NUM_CHUNKS_Z = 6;
    constexpr static i32 TOTAL_BLOCKS_X = NUM_CHUNKS_X * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Y = NUM_CHUNKS_Y * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Z = NUM_CHUNKS_Z * Chunk::NUM_BLOCKS_PER_AXIS;

    Landscape(i32 seed);

    // A landscape cannot be copied.
    Landscape(const Landscape&) = delete;
    Landscape(const Landscape&&) = delete;
    Landscape &operator=(const Landscape&) = delete;
    Landscape &operator=(const Landscape&&) = delete;

    void update_chunk_buffer(i32 cx, i32 cy, i32 cz);
    bool block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi) const;
    void update();
    void generate(f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);

    // inline i32 get_index_from_coords(i32 x, i32 y, i32 z) const
    // {
    //     return (x * NUM_CHUNKS_Y * NUM_CHUNKS_Z + y * NUM_CHUNKS_Z + z);
    // }

public:
    Chunk   chunks[NUM_CHUNKS_X][NUM_CHUNKS_Y][NUM_CHUNKS_Z];
    Vec3f   origin;

private:
    i32           m_seed;
    osn_context  *m_simplex_ctx;

    void initialize_buffers();
};

#endif // __LANDSCAPE_HPP__
