#include "world.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "resource_manager.hpp"
#include "lt_utils.hpp"
#include "input.hpp"
#include "open-simplex-noise.h"
#include "gl_resources.hpp"
#include "texture.hpp"
#include "application.hpp"
#include "vertex.hpp"

lt_global_variable lt::Logger logger("world");

Camera
World::create_camera(Vec3f position, f32 aspect_ratio)
{
    const f32 FIELD_OF_VIEW = 60.0f;
    const f32 MOVE_SPEED = 0.43f;
    const f32 ROTATION_SPEED = 0.033f;
    const Vec3f CAMERA_FRONT(0, 0, -1);
    const Vec3f UP_WORLD(0.0f, 1.0f, 0.0f);
    return Camera(position + Vec3f(0, 40, 0), CAMERA_FRONT, UP_WORLD,
                  FIELD_OF_VIEW, aspect_ratio, MOVE_SPEED, ROTATION_SPEED);
}

World::World(Application &app, i32 seed, const char *blocks_texture,
             const ResourceManager &manager, f32 aspect_ratio)
    : landscape(nullptr)
    , skybox("skybox.texture", "skybox.shader", manager)
    , blocks_texture_info(blocks_texture, manager)
    , sun()
{
    const f64 amplitude = 0.70;
    const f64 frequency = 0.02;
    const i32 num_octaves = 5;
    const f64 lacunarity = 2.0f;
    const f64 gain = 0.5f;

    landscape = std::make_shared<Landscape>(app.memory, seed, amplitude, frequency, num_octaves, lacunarity, gain);
    landscape->generate();

    camera = create_camera(landscape->center(), aspect_ratio);

    sun.direction = lt::normalize(Vec3f(0.49f, -0.35f, 0.80f));
    sun.ambient = Vec3f(.1f);
    sun.diffuse = Vec3f(.7f);
    sun.specular = Vec3f(1.0f);
}

void
World::update(Input &input)
{
    if (skybox.load() && blocks_texture_info.load())
    {
        state = WorldStatus_Running;

        camera.update(input);

        if (input.keys[GLFW_KEY_T].last_transition == Key::Transition_Down)
            render_wireframe = !render_wireframe;

        landscape->update(camera);
    }
}

World
World::interpolate(const World &previous, const World &current, f32 alpha)
{
    LT_Assert(previous.camera.up_world == current.camera.up_world);

    World interpolated = current;
    interpolated.camera = Camera::interpolate(previous.camera, current.camera, alpha);

    return interpolated;
}
