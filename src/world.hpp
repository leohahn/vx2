#ifndef __WORLD_HPP__
#define __WORLD_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"

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

struct World
{
    constexpr static i32 NUM_CHUNKS_PER_AXIS = 1;

    Chunk chunks[NUM_CHUNKS_PER_AXIS][NUM_CHUNKS_PER_AXIS][NUM_CHUNKS_PER_AXIS];
    Vec3f origin;

    void initialize_buffers();
    ~World();
};

#endif // __WORLD_HPP__
