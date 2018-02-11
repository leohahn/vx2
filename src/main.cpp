#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "lt_fs.hpp"
#include "resources.hpp"
#include "application.hpp"
#include "shader.hpp"
#include "world.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "resource_manager.hpp"

lt_internal void
update_key_state(i32 key_code, Key *kb, GLFWwindow *win)
{
    const bool key_pressed = glfwGetKey(win, key_code);
    Key &key = kb[key_code];

    if ((key_pressed && key.is_pressed) || (!key_pressed && !key.is_pressed))
    {
        key.last_transition = Key::Transition_None;
    }
    else if (key_pressed && !key.is_pressed)
    {
        key.is_pressed = true;
        key.last_transition = Key::Transition_Down;
    }
    else if (!key_pressed && key.is_pressed)
    {
        key.is_pressed = false;
        key.last_transition = Key::Transition_Up;
    }
}

lt_internal void
process_input(GLFWwindow *win, Key *kb)
{
    LT_Unused(kb);

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(win, true);
    }

    const i32 key_codes[] = {
        GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_UP,
        GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
    };

    for (auto key_code : key_codes)
        update_key_state(key_code, kb, win);
}

lt_global_variable lt::Logger logger("main");
lt_global_variable Key g_keyboard[NUM_KEYBOARD_KEYS] = {};

lt_internal void
main_update(Key *kb, Camera &camera)
{
    camera.update(kb);
}

lt_internal void
main_render(const Application &app, const World &world, const Camera &camera,
            Shader *basic_shader, Shader *deferred_shading_shader)
{
    app.bind_gbuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_world(world, camera, basic_shader); // render world to the gbuffer

    app.bind_default_framebuffer(); // render gbuffer back to the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_final_quad(app, deferred_shading_shader);

    glfwPollEvents();
    glfwSwapBuffers(app.window);
}

int
main()
{
    ResourceManager resource_manager;
    resource_manager.load_from_file("basic.glsl", ResourceType_Shader);
    resource_manager.load_from_file("deferred_shading.glsl", ResourceType_Shader);

    Resources resources = {};
    Application app("Deferred renderer", 800, 600);

    World world = {};
    world.initialize_buffers();

    world.chunks[0][0][0].blocks[0][0][0] = BlockType_Terrain;

    const f32 FIELD_OF_VIEW = 60.0f;
    const f32 MOVE_SPEED = 0.33f;
    const f32 ROTATION_SPEED = 0.050f;
    const Vec3f CAMERA_POSITION(0, 0, 15);
    const Vec3f CAMERA_FRONT(0, 0, -1);
    const Vec3f UP_WORLD(0.0f, 1.0f, 0.0f);
    Camera camera(CAMERA_POSITION, CAMERA_FRONT, UP_WORLD,
                  FIELD_OF_VIEW, app.aspect_ratio(), MOVE_SPEED, ROTATION_SPEED);

    Shader *basic_shader = resource_manager.get_shader("basic.glsl");
    basic_shader->load();
    basic_shader->setup_projection_matrix(app.aspect_ratio());

    Shader *deferred_shading_shader = resource_manager.get_shader("deferred_shading.glsl");
    deferred_shading_shader->load();
    deferred_shading_shader->add_texture("texture_position");
    deferred_shading_shader->add_texture("texture_normal");
    deferred_shading_shader->add_texture("texture_albedo_specular");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    app.bind_gbuffer();

    bool running = true;
    while (running)
    {
        process_input(app.window, g_keyboard);

        main_update(g_keyboard, camera);

        // Check if the window should close.
        if (glfwWindowShouldClose(app.window))
        {
            running = false;
            continue;
        }

        main_render(app, world, camera, basic_shader, deferred_shading_shader);
    }
}
