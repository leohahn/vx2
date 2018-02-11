#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "application.hpp"
#include "shader.hpp"
#include "world.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "resource_manager.hpp"

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
    render_final_quad(app, camera, deferred_shading_shader);

    glfwPollEvents();
    glfwSwapBuffers(app.window);
}

int
main()
{
    //
    // TODO:
    //   - Add decent main loop with interpolation.
    //   - Add basic shading.
    //   - Create landscape with random noise.
    //
    ResourceManager resource_manager;
    resource_manager.load_from_file("basic.glsl", ResourceType_Shader);
    resource_manager.load_from_file("deferred_shading.glsl", ResourceType_Shader);

    Application app("Deferred renderer", 1024, 768);

    World world;
    world.chunks[0][0][0].blocks[0][0][0] = BlockType_Terrain;

    world.sun.direction = Vec3f(0, -1, 0);
    world.sun.ambient = Vec3f(.1f);
    world.sun.diffuse = Vec3f(.7f);
    world.sun.specular = Vec3f(1.0f);

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
    deferred_shading_shader->use();
    deferred_shading_shader->set3f("sun.direction", world.sun.direction);
    deferred_shading_shader->set3f("sun.ambient", world.sun.ambient);
    deferred_shading_shader->set3f("sun.diffuse", world.sun.diffuse);
    deferred_shading_shader->set3f("sun.specular", world.sun.specular);

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
