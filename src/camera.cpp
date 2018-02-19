#include "camera.hpp"
#include "lt_utils.hpp"
#include "input.hpp"
#include <GLFW/glfw3.h>
#include "world.hpp"

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
    update_frustum_points(frustum);
}

void
Camera::rotate()
{
    const Quatf rotated_front = lt::rotate(frustum.front, rotation_speed, Quatf(0, curr_rotation_axis));
    frustum.front = rotated_front;
    update_frustum_right_and_up(frustum, up_world);
}

Camera
Camera::interpolate(const Camera &previous, const Camera &current, f32 alpha)
{
    Camera interpolated = previous;
    interpolated.frustum.position = previous.frustum.position +
        alpha*(current.frustum.position - previous.frustum.position);

    if (previous.frustum.front == current.frustum.front)
        interpolated.frustum.front = previous.frustum.front;
    else
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

bool
Frustum::is_chunk_inside(const Chunk &chunk) const
{
    Vec3f offset_far_right = right.v * (0.5f * zfar_width);

    Vec3f far_top_left = zfar_center + up.v * (zfar_height * 0.5f) - offset_far_right;
    Vec3f far_top_right = zfar_center + up.v * (zfar_height * 0.5f) + offset_far_right;
    Vec3f far_bottom_left = zfar_center - up.v * (zfar_height * 0.5f) - offset_far_right;
    Vec3f far_bottom_right = zfar_center - up.v * (zfar_height * 0.5f) + offset_far_right;

    Vec3f offset_near_right = right.v * (znear_width * 0.5f);
    Vec3f near_top_left = (znear_center + up.v * (znear_height * 0.5f)) - offset_near_right;
    Vec3f near_top_right = (znear_center + up.v * (znear_height * 0.5f)) + offset_near_right;
    Vec3f near_bottom_left = (znear_center - up.v * (znear_height * 0.5f)) - offset_near_right;
    Vec3f near_bottom_right = (znear_center - up.v * (znear_height * 0.5f)) + offset_near_right;

    Vec3f top_left_vec = far_top_left - near_top_left;
    Vec3f bottom_left_vec = far_bottom_left - near_bottom_left;
    Vec3f top_right_vec = far_top_right - near_top_right;
    Vec3f bottom_right_vec = far_bottom_right - near_bottom_right;
    Vec3f far_h = far_top_left - far_top_right;
    Vec3f far_v = far_bottom_left - far_top_left;
    Vec3f frustum_normals[6] =
    {
        // Right normal
        lt::normalize(lt::cross(bottom_right_vec, top_right_vec)),
        /* UM::Normalize(Vec3f_Cross(farV, topRightVec)), */
        // LEft normal
        lt::normalize(lt::cross(top_left_vec, bottom_left_vec)),
        // TOp normal
        lt::normalize(lt::cross(top_right_vec, top_left_vec)),
        // BOttom normal
        lt::normalize(lt::cross(bottom_left_vec, bottom_right_vec)),
        // FAr normal
        lt::normalize(lt::cross(far_v, far_h)),
        // NEar normal
        lt::normalize(lt::cross(far_h, far_v))
    };
    Vec3f frustum_points[6] =
    {
        far_top_right,
        far_top_left,
        far_top_right,
        far_bottom_right,
        far_bottom_right,
        near_top_left,
    };

    bool inside_frustum = true;
    Vec3f point_vec;
    for (i32 p = 0; p < 6; p++)
    {
        bool insidePlane = false;
        for (i32 v = 0; v < 8; v++)
        {
            point_vec = frustum_points[p] - chunk.max_vertices[v];
            if (lt::dot(point_vec, frustum_normals[p]) >= 0.0f)
            {
                insidePlane = true;
            }
        }
        if (!insidePlane)
        {
            inside_frustum = false;
            break;
        }
    }
    return inside_frustum;
}
