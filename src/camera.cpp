#include "camera.hpp"
#include "lt_utils.hpp"
#include "input.hpp"
#include <GLFW/glfw3.h>
#include "world.hpp"
#include "input.hpp"

static lt::Logger logger("camera");

lt_internal void
update_frustum_points(Frustum &frustum)
{
    f32 fovy_rads = lt::radians(frustum.fovy);
    frustum.znear_center = frustum.position + frustum.front.v * frustum.znear;
    frustum.zfar_center  = frustum.position + frustum.front.v * frustum.zfar;
    frustum.zfar_height  = std::abs(2 * std::tan(fovy_rads / 2) * frustum.zfar);
    frustum.zfar_width   = frustum.zfar_height * frustum.ratio;
    frustum.znear_height = std::abs(2 * std::tan(fovy_rads / 2) * frustum.znear);
    frustum.znear_width  = frustum.znear_height * frustum.ratio;
}

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
    // TODO: Find a better number of split points instead of blindly hardcoding it.
    frustum.split_for_csm(4);

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
Camera::update(Input &input)
{
    curr_direction = Vec3f(0.0f);

    // Register the camera movements from buttons.
    if (input.keys[GLFW_KEY_A].is_pressed)
        add_frame_movement(Direction::Left);

    if (input.keys[GLFW_KEY_D].is_pressed)
        add_frame_movement(Direction::Right);

    if (input.keys[GLFW_KEY_W].is_pressed)
        add_frame_movement(Direction::Forwards);

    if (input.keys[GLFW_KEY_S].is_pressed)
        add_frame_movement(Direction::Backwards);

    move();

    const i32 xoffset = floor(input.mouse_state.xoffset);
    const i32 yoffset = floor(input.mouse_state.yoffset);

    if (xoffset < 0) // move left
    {
        rotate(frustum.up.v, -xoffset);
    }
    else if (xoffset > 0) // move right
    {
        rotate(-frustum.up.v, xoffset);
    }

    if (yoffset < 0) // move up
    {
        rotate(frustum.right.v, -yoffset);
    }
    else if (yoffset > 0) // move down
    {
        rotate(-frustum.right.v, yoffset);
    }

    // Finally move and rotate the camera based on the previous added frame data.
    update_frustum_points(frustum);
}

void
Camera::rotate(Vec3f axis, i32 times)
{
    LT_Assert(times > 0);
    const Quatf rotated_front = lt::rotate(frustum.front,
                                           rotation_speed*times, Quatf(0, axis));
    frustum.front = rotated_front;
    update_frustum_right_and_up(frustum, up_world);
}

Camera
Camera::interpolate(const Camera &previous, const Camera &current, f32 alpha)
{
    Camera interpolated = previous;
    interpolated.frustum.position = previous.frustum.position +
        alpha*(current.frustum.position - previous.frustum.position);

    interpolated.frustum.front = lt::slerp(previous.frustum.front, current.frustum.front, alpha);

    update_frustum_right_and_up(interpolated.frustum, previous.up_world);

    return interpolated;
}

Mat4f
Camera::view_matrix() const
{
    return lt::look_at(frustum.position,
                       frustum.position + frustum.front.v,
                       frustum.up.v);
}

// ======================================================================================
// Frustum
// ======================================================================================

void
Frustum::split_for_csm(i32 num_split_points)
{
    // Formula for splitting the points
    // zi = lambda*near(far/near)^(i/N) + (1-lambda)(near+(i/N)(far-near))

    logger.log("Splitting frustum for CSM");

    const auto do_split = [num_split_points](i32 index, f32 n, f32 f) {
        constexpr f32 lambda = 0.9f;
        const f32 first_term = lambda*n*std::pow(f/n, static_cast<f32>(index)/num_split_points);
        const f32 second_term = (1-lambda)*(n+(index/num_split_points)*(f-n));
        const f32 z = first_term + second_term;
        return z;
    };

    std::vector<f32> z_splits(num_split_points);
    for (i32 i = 0; i < num_split_points; i++)
    {
        z_splits[i] = do_split(i+1, Camera::ZNEAR, Camera::ZFAR);
        logger.log("Split for ", i, " = ", z_splits[i]);
    }
}
