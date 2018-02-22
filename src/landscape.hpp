#ifndef __LANDSCAPE_HPP__
#define __LANDSCAPE_HPP__

#include <list>

#include "lt_core.hpp"
#include "lt_math.hpp"

struct TextureAtlas;
struct Camera;
struct ResourceManager;
struct osn_context;

enum BlockType
{
    BlockType_Air,
    BlockType_Terrain,
    BlockType_Count,
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

typedef i32 ChunkIndex;

struct Chunk
{
    constexpr static i32 NUM_BLOCKS_PER_AXIS = 16;
    constexpr static i32 NUM_BLOCKS = NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS;
    constexpr static i32 BLOCK_SIZE = 1;
    constexpr static i32 SIZE = BLOCK_SIZE * NUM_BLOCKS_PER_AXIS;

    Chunk(Vec3f origin, BlockType fill_type = BlockType_Air);
    ~Chunk();
    Chunk(Chunk &chunk) = delete;
    Chunk &operator=(const Chunk &chunk) = delete;

public:
    BlockType blocks[NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS];
    Vec3f     max_vertices[8];
    Vec3f     origin;
    u32       vao, vbo;
    // Signals that the chunk's buffer must be updated.
    i32       num_vertices_used;
    bool      outdated;
};

struct Landscape
{
    constexpr static i32 NUM_CHUNKS_X = 4;
    constexpr static i32 NUM_CHUNKS_Y = 4;
    constexpr static i32 NUM_CHUNKS_Z = 4;
    constexpr static i32 TOTAL_BLOCKS_X = NUM_CHUNKS_X * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Y = NUM_CHUNKS_Y * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Z = NUM_CHUNKS_Z * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 SIZE_X = TOTAL_BLOCKS_X * Chunk::BLOCK_SIZE;
    constexpr static i32 SIZE_Y = TOTAL_BLOCKS_Y * Chunk::BLOCK_SIZE;
    constexpr static i32 SIZE_Z = TOTAL_BLOCKS_Z * Chunk::BLOCK_SIZE;

    Landscape(i32 seed, f64 amplitude, f64 frequency, i32 num_octaves, f64 lacunarity, f64 gain);

    // A landscape cannot be copied.
    Landscape(const Landscape&) = delete;
    Landscape(const Landscape&&) = delete;
    Landscape &operator=(const Landscape&) = delete;
    Landscape &operator=(const Landscape&&) = delete;

    void update_chunk_buffer(i32 cx, i32 cy, i32 cz);
    bool block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi) const;
    void update(const Camera &camera);
    void generate();
    void generate_for_chunk(Chunk &chunk);

    inline Vec3f center() const
    {
        return origin + 0.5f*Vec3f(SIZE_X, SIZE_Y, SIZE_Z);
    }

public:
    std::list<Chunk> chunks;
    std::list<Chunk>::iterator chunk_ptrs[NUM_CHUNKS_X][NUM_CHUNKS_Y][NUM_CHUNKS_Z];
    Vec3f   origin;

private:
    i32           m_seed;
    f64           m_amplitude;
    f64           m_frequency;
    i32           m_num_octaves;
    f64           m_lacunarity;
    f64           m_gain;

    osn_context  *m_simplex_ctx;


    void initialize_chunks();
    Vec3f get_chunk_origin(i32 cx, i32 cy, i32 cz);
};

#endif // __LANDSCAPE_HPP__
