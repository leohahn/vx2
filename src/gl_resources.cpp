#include "gl_resources.hpp"
#include "glad/glad.h"
#include "lt_utils.hpp"

lt_global_variable lt::Logger logger("gl_resources");

u32
GLResources::create_buffer()
{
    u32 buf;
    glGenBuffers(1, &buf);
    LT_Assert(m_buffers.find(buf) == m_buffers.end());
    m_buffers.insert({buf, 1});
    return buf;
}

u32
GLResources::create_vertex_array()
{
    u32 vao;
    glGenVertexArrays(1, &vao);

    auto it = m_vertex_arrays.find(vao);
    LT_Assert(it == m_vertex_arrays.end());
    m_vertex_arrays.insert({vao, 1});
    return vao;
}

void
GLResources::add_reference_to_vertex_array(u32 v)
{
    LT_Assert(m_vertex_arrays[v] > 0);
    m_vertex_arrays[v]++;
}

void
GLResources::add_reference_to_buffer(u32 b)
{
    LT_Assert(m_buffers[b] > 0);
    m_buffers[b]++;
}

void
GLResources::delete_buffer(u32 b)
{
    LT_Assert(m_buffers[b] > 0);
    m_buffers[b]--;

    if (m_buffers[b] == 0)
    {
        glDeleteBuffers(1, &b);
    }
}

void
GLResources::delete_vertex_array(u32 v)
{
    LT_Assert(m_vertex_arrays[v] > 0);
    m_vertex_arrays[v]--;

    if (m_vertex_arrays[v] == 0)
    {
        glDeleteVertexArrays(1, &v);
    }
}
