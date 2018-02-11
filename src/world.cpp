#include "world.hpp"
#include "glad/glad.h"

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

World::~World()
{
    for (i32 x = 0; x < NUM_CHUNKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_CHUNKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_CHUNKS_PER_AXIS; z++)
                glDeleteBuffers(1, &chunks[x][y][z].vbo);
}
