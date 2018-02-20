#ifndef __GL_RESOURCES_HPP__
#define __GL_RESOURCES_HPP__

#include <unordered_map>
#include "lt_core.hpp"

struct GLResources
{
    GLResources(GLResources&) = delete;
    GLResources &operator=(GLResources &) = delete;
    static GLResources &instance()
    {
        static GLResources instance;
        return instance;
    }

    u32 create_buffer();
    u32 create_vertex_array();

    void add_reference_to_vertex_array(u32 vertex_array);
    void add_reference_to_buffer(u32 buffer);

    void delete_buffer(u32 buffer);
    void delete_vertex_array(u32 vertex_array);
private:
    GLResources() {};

    std::unordered_map<u32, i32> m_vertex_arrays;
    std::unordered_map<u32, i32> m_buffers;
};

#endif // __GL_RESOURCES_HPP__
