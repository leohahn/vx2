#include "world.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "resource_manager.hpp"
#include "lt_utils.hpp"
#include "input.hpp"

lt_global_variable lt::Logger logger("world");

void
World::initialize_buffers()
{
    for (i32 x = 0; x < NUM_CHUNKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_CHUNKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_CHUNKS_PER_AXIS; z++)
            {
                glGenVertexArrays(1, &chunks[x][y][z].vao);
                glGenBuffers(1, &chunks[x][y][z].vbo);
            }
}

Camera
World::create_camera(f32 aspect_ratio)
{
    const f32 FIELD_OF_VIEW = 60.0f;
    const f32 MOVE_SPEED = 0.13f;
    const f32 ROTATION_SPEED = 0.033f;
    const Vec3f CAMERA_POSITION(0, 0, 15);
    const Vec3f CAMERA_FRONT(0, 0, -1);
    const Vec3f UP_WORLD(0.0f, 1.0f, 0.0f);
    return Camera(CAMERA_POSITION, CAMERA_FRONT, UP_WORLD,
                  FIELD_OF_VIEW, aspect_ratio, MOVE_SPEED, ROTATION_SPEED);
}

World::World(const ResourceManager &manager, f32 aspect_ratio)
    : camera(create_camera(aspect_ratio))
    , chunks()
    , skybox("skybox.texture", "skybox.glsl", manager)
    , sun()
{
    initialize_buffers();

    sun.direction = Vec3f(0, -1, 0);
    sun.ambient = Vec3f(.1f);
    sun.diffuse = Vec3f(.7f);
    sun.specular = Vec3f(1.0f);
}

World::~World()
{
    for (i32 x = 0; x < NUM_CHUNKS_PER_AXIS; x++)
        for (i32 y = 0; y < NUM_CHUNKS_PER_AXIS; y++)
            for (i32 z = 0; z < NUM_CHUNKS_PER_AXIS; z++)
                glDeleteBuffers(1, &chunks[x][y][z].vbo);
}

void
World::update(Key *kb)
{
    if (skybox.load())
    {
        state = WorldState_Running;
    }

    camera.update(kb);

    if (kb[GLFW_KEY_T].last_transition == Key::Transition_Down)
        render_wireframe = !render_wireframe;
}
