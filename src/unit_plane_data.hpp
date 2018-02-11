#ifndef __UNIT_PLANE_DATA_HPP__
#define __UNIT_PLANE_DATA_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"

lt_internal const Vec3f UNIT_PLANE_VERTICES[] =
{
    Vec3f(-1.0f, -1.0f,  0.0f), // 0
    Vec3f( 1.0f, -1.0f,  0.0f), // 1
    Vec3f( 1.0f,  1.0f,  0.0f), // 2
    Vec3f(-1.0f,  1.0f,  0.0f), // 3
};

lt_internal const Vec2f UNIT_PLANE_TEX_COORDS[] =
{
    Vec2f(0.0f, 0.0f), // 0
    Vec2f(1.0f, 0.0f), // 1
    Vec2f(1.0f, 1.0f), // 2
    Vec2f(0.0f, 1.0f), // 3
};

static_assert(LT_Count(UNIT_PLANE_VERTICES) == LT_Count(UNIT_PLANE_TEX_COORDS),
              "vertices and tex_coords should be the same size");

lt_internal const u32 UNIT_PLANE_INDICES[] =
{
    0, 1, 2, 2, 3, 0
};

#endif // __UNIT_PLANE_DATA_HPP__
