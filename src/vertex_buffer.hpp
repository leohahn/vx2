#ifndef __VERTEX_BUFFER_HPP__
#define __VERTEX_BUFFER_HPP__

#include "lt_core.hpp"

struct Mesh;

namespace VertexBuffer
{

void setup_pu(Mesh &mesh);
void setup_pl(Mesh &m, i32 layer);

}

#endif // __VERTEX_BUFFER_HPP__
