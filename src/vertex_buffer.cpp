#include "vertex_buffer.hpp"
#include "glad/glad.h"
#include <vector>
#include "vertex.hpp"
#include "mesh.hpp"

namespace VertexBuffer
{

void
setup_pu(Mesh &m)
{
    //
    // PERFORMANCE: consider using a preallocated buffer or custom allocator for the following vector.
    //
    std::vector<Vertex_PU> vertexes_buf(m.vertices.size());
    for (usize i = 0; i < m.vertices.size(); i++)
    {
        vertexes_buf[i].position = m.vertices[i];
        vertexes_buf[i].tex_coords = m.tex_coords[i];
    }

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexes_buf.size() * sizeof(Vertex_PU), &vertexes_buf[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.faces.size() * sizeof(Face), &m.faces[0], GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU),
                          (void*)offsetof(Vertex_PU, position));
    glEnableVertexAttribArray(0);
    // vertex texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU),
                          (void*)offsetof(Vertex_PU, tex_coords));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void
setup_pl(Mesh &m, i32 layer)
{
    //
    // PERFORMANCE: consider using a preallocated buffer or custom allocator for the following vector.
    //
    std::vector<Vertex_PL> vertexes_buf(m.vertices.size());
    for (usize i = 0; i < m.vertices.size(); i++)
    {
        vertexes_buf[i].position = m.vertices[i];
        vertexes_buf[i].tex_coords_layer = Vec3f(m.tex_coords[i], layer);
    }

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexes_buf.size() * sizeof(Vertex_PL), &vertexes_buf[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.faces.size() * sizeof(Face), &m.faces[0], GL_STATIC_DRAW);

    // vertex positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PL),
                          (void*)offsetof(Vertex_PL, position));
    glEnableVertexAttribArray(0);
    // vertex texture coords
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PL),
                          (void*)offsetof(Vertex_PL, tex_coords_layer));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

}
