#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "lt_math.hpp"

struct Key;
struct Chunk;
struct Input;

struct Frustum
{
    Vec3f  position;
    f32    ratio;
    f32    fovy;

    f32    znear;
    f32    znear_width;
    f32    znear_height;
    Vec3f  znear_center;

    f32    zfar;
    f32    zfar_width;
    f32    zfar_height;
    Vec3f  zfar_center;

    // Calculated attributes
    Quatf  front;
    Quatf  right;
    Quatf  up;
    Vec3f  normals[6];
    Mat4f  projection;

    // bool   is_chunk_inside(const Chunk &chunk) const;
};

struct Camera
{
    static constexpr f32 ZNEAR = 0.1f;
    static constexpr f32 ZFAR = 1000.0f;
    static Camera interpolate(const Camera &previous, const Camera &current, f32 alpha);

    enum class Direction { Left, Right, Forwards, Backwards };
    enum class RotationAxis { Up, Down, Right, Left };

    Camera();
    Camera(Vec3f position, Vec3f front_vec, Vec3f up_world,
           f32 fovy, f32 ratio, f32 move_speed, f32 rotation_speed);

    void update(Input &input);
    void interpolate_frustum(f64 lag_offset);
    inline Vec3f position() const { return frustum.position; }
    inline Vec3f front() const { return frustum.front.v; }

    Mat4f view_matrix() const;

public:
    Frustum frustum;
    Vec3f   up_world;
    f32     move_speed;
    f32     rotation_speed;
    Vec3f   curr_direction;

private:
    void add_frame_movement(Direction dir);

    inline void move() { frustum.position += (curr_direction * move_speed); }
    void rotate(Vec3f axis, i32 times);
};

#endif // CAMERA_HPP
