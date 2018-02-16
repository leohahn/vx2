#include "camera.hpp"
#include "lt_utils.hpp"
#include "input.hpp"
#include <GLFW/glfw3.h>

static lt::Logger logger("camera");

lt_internal void
update_frustum_right_and_up(Frustum& frustum, Vec3f up_world)
{
    const Vec3f right_vec = lt::normalize(lt::cross(frustum.front.v, up_world));
    const Vec3f up_vec = lt::normalize(lt::cross(right_vec, frustum.front.v));

    frustum.right = Quatf(0, right_vec);
    frustum.up = Quatf(0, up_vec);
}

Camera::Camera()
    : up_world(Vec3f(0))
    , move_speed(0)
    , rotation_speed(0)
{
}

Camera::Camera(Vec3f position, Vec3f front_vec, Vec3f up_world,
               f32 fovy, f32 ratio, f32 move_speed, f32 rotation_speed)
    : up_world(up_world)
    , move_speed(move_speed)
    , rotation_speed(rotation_speed)
{
    logger.log("initializing frustum");
    frustum.front = Quatf(0, lt::normalize(front_vec));
    frustum.position = position;
    frustum.ratio = ratio;
    frustum.fovy = fovy;
    frustum.znear = ZNEAR;
    frustum.zfar = ZFAR;
    frustum.projection = lt::perspective(lt::radians(fovy), ratio, ZNEAR, ZFAR);

    update_frustum_right_and_up(frustum, up_world);
}

void
Camera::add_frame_movement(Direction dir)
{
    switch (dir)
    {
    case Direction::Left: curr_direction = lt::normalize(curr_direction - frustum.right.v); break;
    case Direction::Right: curr_direction = lt::normalize(curr_direction + frustum.right.v); break;
    case Direction::Forwards: curr_direction = lt::normalize(curr_direction + frustum.front.v); break;
    case Direction::Backwards: curr_direction = lt::normalize(curr_direction - frustum.front.v); break;
    default: LT_Assert(false);
    }
}

void
Camera::add_frame_rotation(RotationAxis axis)
{
    switch (axis)
    {
    case RotationAxis::Up:
        curr_rotation_axis = lt::normalize(curr_rotation_axis + frustum.up.v);
        break;
    case RotationAxis::Down:
        curr_rotation_axis = lt::normalize(curr_rotation_axis - frustum.up.v);
        break;
    case RotationAxis::Right:
        curr_rotation_axis = lt::normalize(curr_rotation_axis + frustum.right.v);
        break;
    case RotationAxis::Left:
        curr_rotation_axis = lt::normalize(curr_rotation_axis - frustum.right.v);
        break;
    default: LT_Assert(false);
    }
}

void
Camera::update(Key *kb)
{
    reset();

    // Register the camera movements from buttons.
    if (kb[GLFW_KEY_A].is_pressed)
        add_frame_movement(Direction::Left);

    if (kb[GLFW_KEY_D].is_pressed)
        add_frame_movement(Direction::Right);

    if (kb[GLFW_KEY_W].is_pressed)
        add_frame_movement(Direction::Forwards);

    // Register the camera rotation axis from the buttons
    if (kb[GLFW_KEY_S].is_pressed)
        add_frame_movement(Direction::Backwards);

    if (kb[GLFW_KEY_RIGHT].is_pressed)
        add_frame_rotation(RotationAxis::Down);

    if (kb[GLFW_KEY_LEFT].is_pressed)
        add_frame_rotation(RotationAxis::Up);

    if (kb[GLFW_KEY_UP].is_pressed)
        add_frame_rotation(RotationAxis::Right);

    if (kb[GLFW_KEY_DOWN].is_pressed)
        add_frame_rotation(RotationAxis::Left);

    // Finally move and rotate the camera based on the previous added frame data.
    move();
    rotate();
}

void
Camera::rotate()
{
    const Quatf rotated_front = lt::rotate(frustum.front, rotation_speed, Quatf(0, curr_rotation_axis));
    frustum.front = rotated_front;
    update_frustum_right_and_up(frustum, up_world);
}

Frustum
Frustum::interpolate(const Frustum &previous, const Frustum &current, f32 alpha, Vec3f up_world)
{
    Frustum interpolated = previous;

    interpolated.position = previous.position*(1.0-alpha) + current.position*alpha;

    if (previous.front == current.front)
        interpolated.front = previous.front;
    else
        interpolated.front = lt::slerp(previous.front, current.front, alpha);

    update_frustum_right_and_up(interpolated, up_world);

    return interpolated;
}

Mat4f
Camera::view_matrix() const
{
    return lt::look_at(frustum.position,
                       frustum.position + frustum.front.v,
                       frustum.up.v);
}
