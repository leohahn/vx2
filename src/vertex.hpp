#ifndef __VERTEX_HPP__
#define __VERTEX_HPP__

#include "lt_math.hpp"

//
// Suffixes:
// P -> position
// U -> vec2(u, v) coordinates
// L -> vec3(u, v, layer) coordinates
// N -> normal
//

struct Vertex_PU
{
    Vec3f position;
    Vec2f tex_coords;

    Vertex_PU(Vec3f p, Vec2f t) : position(p), tex_coords(t) {}
    Vertex_PU() : position(Vec3f(0)), tex_coords(Vec2f(0)) {}
};

struct Vertex_PL
{
    Vec3f position;
    Vec3f tex_coords_layer;

    Vertex_PL(Vec3f p, Vec3f t) : position(p), tex_coords_layer(t) {}
    Vertex_PL() : position(0), tex_coords_layer(0) {}
};

struct Vertex_PUN
{
    Vec3f position;
    Vec2f tex_coords;
    Vec3f normal;
};

struct Vertex_PLN
{
    Vec3f position         = Vec3f(0.0f);
    Vec3f tex_coords_layer = Vec3f(0.0f);
    Vec3f normal           = Vec3f(0.0f);
};

#endif // __VERTEX_HPP__
