#ifndef __UNIT_CUBE_DATA_HPP__
#define __UNIT_CUBE_DATA_HPP__

#include "lt_core.hpp"
#include "lt_math.hpp"

lt_internal const u32 UNIT_CUBE_INDICES[] =
{
    0,  1,  2,  2,  3,  0,
    4,  5,  6,  6,  7,  4,
    8,  9, 10, 10, 11,  8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20,
};

lt_internal const Vec3f UNIT_CUBE_VERTICES[24] =
{
    // FRONT
    Vec3f(-1, -1,  1), // 0
    Vec3f( 1, -1,  1), // 1
    Vec3f( 1,  1,  1), // 2
    Vec3f(-1,  1,  1), // 3
    // BACK
    Vec3f( 1, -1, -1), // 4
    Vec3f(-1, -1, -1), // 5
    Vec3f(-1,  1, -1), // 6
    Vec3f( 1,  1, -1), // 7
    // TOP
    Vec3f(-1,  1,  1), // 8
    Vec3f( 1,  1,  1), // 9
    Vec3f( 1,  1, -1), // 10
    Vec3f(-1,  1, -1), // 11
    // BOTTOM
    Vec3f( 1, -1,  1), // 12
    Vec3f(-1, -1,  1), // 13
    Vec3f(-1, -1, -1), // 14
    Vec3f( 1, -1, -1), // 15
    // LEFT
    Vec3f(-1, -1, -1), // 16
    Vec3f(-1, -1,  1), // 17
    Vec3f(-1,  1,  1), // 18
    Vec3f(-1,  1, -1), // 19
    // RIGHT
    Vec3f( 1, -1,  1), // 20
    Vec3f( 1, -1, -1), // 21
    Vec3f( 1,  1, -1), // 22
    Vec3f( 1,  1,  1), // 23
};

lt_internal const Vec2f UNIT_CUBE_TEX_COORDS[24] =
{
    // FRONT
    Vec2f(0, 0), // 0
    Vec2f(1, 0), // 1
    Vec2f(1, 1), // 2
    Vec2f(0, 1), // 3
    // BACK
    Vec2f(0, 0), // 4
    Vec2f(1, 0), // 5
    Vec2f(1, 1), // 6
    Vec2f(0, 1), // 7
    // TOP
    Vec2f(0, 0), // 8
    Vec2f(1, 0), // 9
    Vec2f(1, 1), // 10
    Vec2f(0, 1), // 11
    // BOTTOM
    Vec2f(0, 0), // 12
    Vec2f(1, 0), // 13
    Vec2f(1, 1), // 14
    Vec2f(0, 1), // 15
    // LEFT
    Vec2f(0, 0), // 16
    Vec2f(1, 0), // 17
    Vec2f(1, 1), // 18
    Vec2f(0, 1), // 19
    // RIGHT
    Vec2f(0, 0), // 20
    Vec2f(1, 0), // 21
    Vec2f(1, 1), // 22
    Vec2f(0, 1), // 23
};

static_assert(LT_Count(UNIT_CUBE_VERTICES) == LT_Count(UNIT_CUBE_TEX_COORDS),
              "need the same amount of vertices");

#endif // __UNIT_CUBE_DATA_HPP__
