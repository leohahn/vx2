#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <chrono>

#include "lt_core.hpp"
#include "lt_utils.hpp"
#include "application.hpp"
#include "shader.hpp"
#include "world.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "io_task_manager.hpp"
#include "resource_manager.hpp"
#include "skybox.hpp"
#include "font.hpp"
#include "gl_resources.hpp"

struct DebugContext
{
    i32 fps;
    i32 ups;
    i32 max_frame_time;
    i32 min_frame_time;
};

using namespace std::chrono_literals;

lt_global_variable lt::Logger logger("main");
lt_global_variable Key g_keyboard[NUM_KEYBOARD_KEYS] = {};
lt_global_variable DebugContext g_debug_context = {};

lt_internal void
main_render(const Application &app, World &world, ResourceManager &resource_manager)
{
    Shader *basic_shader = resource_manager.get_shader("basic.shader");
    Shader *wireframe_shader = resource_manager.get_shader("wireframe.shader");
    Shader *font_shader = resource_manager.get_shader("font.shader");
    AsciiFontAtlas *font_atlas = resource_manager.get_font("dejavu/ttf/DejaVuSansMono.ttf");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    app.bind_default_framebuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (world.state == WorldStatus_InitialLoad)
    {
        render_loading_screen(app, font_atlas, font_shader);
        dump_opengl_errors("After loading screen", __FILE__);
    }
    else
    {
        if (world.render_wireframe)
        {
            // Wireframe rendering
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            wireframe_shader->use();
            wireframe_shader->set_matrix("view", world.camera.view_matrix());
            render_world(world);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else
        {
            basic_shader->use();
            basic_shader->set_matrix("view", world.camera.view_matrix());
            basic_shader->set3f("view_position", world.camera.frustum.position);
            glActiveTexture(GL_TEXTURE0 + basic_shader->texture_unit("texture_array_blocks"));
            glBindTexture(GL_TEXTURE_2D_ARRAY, world.blocks_texture_info.texture_id());
            render_world(world); // render world to the gbuffer
        }

        // Draw the skybox
        world.skybox.shader->use();
        world.skybox.shader->set_matrix("view", world.camera.view_matrix());
        render_skybox(world.skybox);
        // dump_opengl_errors("After render_skybox", __FILE__);

        lt_local_persist char text_buffer[256] = {};
        snprintf(text_buffer, sizeof(text_buffer),
                 "FPS: %d, UPS: %d -- Frame time: %.2f min | %.2f max\n"
                 "Camera: (%.2f, %.2f, %.2f) -- Front: (%.2f, %.2f, %.2f)",
                 g_debug_context.fps,
                 g_debug_context.ups,
                 (f32)g_debug_context.min_frame_time,
                 (f32)g_debug_context.max_frame_time,
                 world.camera.position().x,
                 world.camera.position().y,
                 world.camera.position().z,
                 world.camera.frustum.front.v.x,
                 world.camera.frustum.front.v.y,
                 world.camera.frustum.front.v.z);

        render_text(font_atlas, text_buffer, 30.5f, 30.5f, font_shader);

        dump_opengl_errors("After font", __FILE__);
    }

    glfwPollEvents();
    glfwSwapBuffers(app.window);
}

int
main()
{
    // --------------------------------------------------------------
    // TODO:
    //   - Load new chunks in separate threads.
    //   - Add frustum culling
    //   - Reduce number of polygons needed to render the world!!
    // --------------------------------------------------------------

    Application app("Deferred renderer", 1680, 1050);
    IOTaskManager io_task_manager;

    ResourceManager resource_manager(&io_task_manager);
    {
        const char *shaders_to_load[] = {
            "basic.shader",
            "wireframe.shader",
            "skybox.shader",
            "font.shader",
        };
        const char *textures_to_load[] = {
            "skybox.texture", "blocks.texture",
        };
        const char *fonts_to_load[] = {
            "dejavu/ttf/DejaVuSansMono.ttf",
        };

        for (auto name : shaders_to_load)
            resource_manager.load_from_shader_file(name);

        for (auto name : textures_to_load)
            resource_manager.load_from_texture_file(name);

        for (auto name : fonts_to_load)
            resource_manager.load_from_font_file(name);
    }

    const i32 seed = -13123;
    World world(seed, "blocks.texture", resource_manager, app.aspect_ratio());

    Shader *basic_shader = resource_manager.get_shader("basic.shader");
    basic_shader->load();
    basic_shader->setup_perspective_matrix(app.aspect_ratio());
    basic_shader->use();
    basic_shader->set3f("sun.direction", world.sun.direction);
    basic_shader->set3f("sun.ambient", world.sun.ambient);
    basic_shader->set3f("sun.diffuse", world.sun.diffuse);
    basic_shader->set3f("sun.specular", world.sun.specular);

    Shader *wireframe_shader = resource_manager.get_shader("wireframe.shader");
    wireframe_shader->load();
    wireframe_shader->setup_perspective_matrix(app.aspect_ratio());

    Shader *font_shader = resource_manager.get_shader("font.shader");
    font_shader->setup_orthographic_matrix(0, app.screen_width, app.screen_height, 0);

    Shader *skybox_shader = resource_manager.get_shader("skybox.shader");
    skybox_shader->setup_perspective_matrix(app.aspect_ratio());

    AsciiFontAtlas *font_atlas = resource_manager.get_font("dejavu/ttf/DejaVuSansMono.ttf");
    LT_Assert(font_atlas);
    font_atlas->load(18.0f);

    //
    // Here starts the setup for the main loop
    //
    bool running = true;

    dump_opengl_errors("Before loop", __FILE__);

    // Define variables to control time
    using clock = std::chrono::high_resolution_clock;

    constexpr std::chrono::nanoseconds TIMESTEP(16ms); // Fixed timestep used for the updates
    std::chrono::nanoseconds lag(0ns);
    auto previous_time = clock::now();

    // Debug variables
    i32 num_updates = 0; // used for counting frames per second
    i32 num_frames = 0;  // used for counting updates per second
    auto start_second = clock::now(); // count seconds
    std::chrono::milliseconds min_frame_time(10000ms);
    std::chrono::milliseconds max_frame_time(0ms);

    World current_world = world;
    World previous_world = world;

    while (running)
    {
        // Update frame information.
        {
            auto current_time = clock::now();
            auto frame_time = current_time - previous_time;
            previous_time = current_time;
            lag += std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time);

            if (frame_time > max_frame_time)
                max_frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time);
            if (frame_time < min_frame_time)
                min_frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time);
        }

        // Check if the window should close.
        if (glfwWindowShouldClose(app.window))
        {
            running = false;
            continue;
        }

        while (lag >= TIMESTEP)
        {
            process_input(app.window, g_keyboard);

            previous_world = current_world;
            current_world.update(g_keyboard);

            num_updates++;
            lag -= TIMESTEP;
        }

        // Interpolate world state based on the current frame lag.
        const f32 lag_offset = (f32)lag.count() / TIMESTEP.count();
        World interpolated_world = World::interpolate(previous_world, current_world, lag_offset);

        // Render the interpolated state.
        main_render(app, interpolated_world, resource_manager);
        num_frames++;

        // Update debug information after one second.
        if (clock::now() - start_second >= 1000ms)
        {
            g_debug_context.fps = num_frames;
            g_debug_context.ups = num_updates;
            g_debug_context.max_frame_time = max_frame_time.count();
            g_debug_context.min_frame_time = min_frame_time.count();

            max_frame_time = 0ms;
            min_frame_time = 10000ms;
            num_frames = 0;
            num_updates = 0;
            start_second = clock::now();
        }
    }

    io_task_manager.stop();
    io_task_manager.join_thread();
}
