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
#include "resource_names.hpp"

#ifdef LT_DEBUG
#include <fenv.h>
#endif

struct DebugContext
{
    i32 fps;
    i32 ups;
    i32 max_frame_time;
    i32 min_frame_time;
    bool render_shadow_map;
    bool render_cascaded_frustum;
    bool render_wireframe;
};

using namespace std::chrono_literals;

lt_global_variable lt::Logger logger("main");
lt_global_variable DebugContext g_debug_context = {};

lt_internal void
main_render_paused(const Application &app, UiRenderer &ui_renderer, const UiState &ui_state)
{
    const Vec3f item_color(1.0f);
    const Vec3f selected_color(1.0f, 0.0f, 0.0);

    // Clear screen.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Add blending and remove depth test.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ui_renderer.begin();
    f32 ypos = 0.45f*app.screen_height;
    f32 xpos = 0.46f*app.screen_width;

    if (ui_state.current_selection == UiState::Selection_Resume)
        ui_renderer.text("Resume", selected_color, xpos, ypos);
    else
        ui_renderer.text("Resume", item_color, xpos, ypos);

    ypos += 60.0f;

    if (ui_state.current_selection == UiState::Selection_Quit)
        ui_renderer.text("Quit", selected_color, xpos, ypos);
    else
        ui_renderer.text("Quit", item_color, xpos, ypos);

    ui_renderer.flush();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    dump_opengl_errors("After paused screen");

    glfwSwapBuffers(app.window);
}

lt_internal void
main_render_loading(const Application &app, ResourceManager &resource_manager)
{
    AsciiFontAtlas *font_atlas = resource_manager.get_font(names::UI_FONT);
    Shader *font_shader = resource_manager.get_shader(names::FONT_SHADER);

    // Add blending and remove depth test.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_loading_screen(app, font_atlas, font_shader);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    dump_opengl_errors("After loading screen");

    glfwSwapBuffers(app.window);
}

lt_internal void
main_render_running(const Application &app, World &world,
                    const ShadowMap &shadow_map, ResourceManager &resource_manager)
{
    Shader *basic_shader = resource_manager.get_shader(names::BASIC_SHADER);
    Shader *wireframe_shader = resource_manager.get_shader(names::WIREFRAME_SHADER);
    Shader *font_shader = resource_manager.get_shader(names::FONT_SHADER);
    AsciiFontAtlas *font_atlas = resource_manager.get_font(names::DEBUG_FONT);

    app.bind_default_framebuffer();
    glClearColor(world.sky_color.r, world.sky_color.g, world.sky_color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (g_debug_context.render_wireframe)
    {
        // Wireframe rendering
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        wireframe_shader->use();
        wireframe_shader->set_matrix("view", world.camera.view_matrix());
        render_landscape(world);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else
    {
        // Render world to the shadow map texture
        {
            glViewport(0, 0, shadow_map.width, shadow_map.height);
            shadow_map.bind_framebuffer();
            glClear(GL_DEPTH_BUFFER_BIT);
            glDisable(GL_CULL_FACE);
            shadow_map.shader->use();
            shadow_map.shader->debug_validate();
            render_landscape(world);
            glEnable(GL_CULL_FACE);
        }

        glViewport(0, 0, app.screen_width, app.screen_height);
        app.bind_default_framebuffer();

        if (g_debug_context.render_shadow_map)
        {
            shadow_map.debug_render_shader->use();
            shadow_map.debug_render_shader->activate_and_bind_texture("texture_shadow_map",
                                                                      GL_TEXTURE_2D, shadow_map.texture);
            shadow_map.debug_render_shader->set_matrix("light_space", world.sun.light_space());
            shadow_map.debug_render_shader->debug_validate();
            render_mesh(shadow_map.debug_render_quad, shadow_map.debug_render_shader);

            glfwSwapBuffers(app.window);
            return;
        }
        else
        {
            basic_shader->use();
            basic_shader->set_matrix("view", world.camera.view_matrix());
            basic_shader->set_matrix("light_space", world.sun.light_space());
            basic_shader->set3f("view_position", world.camera.frustum.position);
            basic_shader->activate_and_bind_texture("texture_array", GL_TEXTURE_2D_ARRAY,
                                                    world.textures_16x16->id);
            basic_shader->activate_and_bind_texture("texture_shadow_map", GL_TEXTURE_2D,
                                                    shadow_map.texture);
            basic_shader->debug_validate();
            render_landscape(world);
        }
    }

    // Draw the skybox
#if 0
    world.skybox.shader->use();
    world.skybox.shader->set_matrix("view", world.camera.view_matrix());
    render_skybox(world.skybox);
#endif

    // Add blending and remove depth test.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the crosshair
    world.crosshair.shader->use();
    render_mesh(world.crosshair.quad, world.crosshair.shader);

    lt_local_persist char text_buffer[256] = {};
    snprintf(text_buffer, LT_Count(text_buffer),
             "FPS: %d, UPS: %d -- Frame time: %.2f min | %.2f max\n"
             "Camera: (%.2f, %.2f, %.2f) -- Front: (%.2f, %.2f, %.2f)\n"
             "Sun: (%.2f, %.2f, %.2f) -- Dir: (%.2f, %.2f, %.2f)",
             g_debug_context.fps,
             g_debug_context.ups,
             (f32)g_debug_context.min_frame_time,
             (f32)g_debug_context.max_frame_time,
             world.camera.position().x,
             world.camera.position().y,
             world.camera.position().z,
             world.camera.frustum.front.v.x,
             world.camera.frustum.front.v.y,
             world.camera.frustum.front.v.z,
             world.sun.position.x,
             world.sun.position.y,
             world.sun.position.z,
             world.sun.direction.x,
             world.sun.direction.y,
             world.sun.direction.z);

    render_text(font_atlas, text_buffer, 30.5f, 30.5f, font_shader);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    dump_opengl_errors("After font");

    glfwSwapBuffers(app.window);
}

lt_internal void
main_render(const Application &app, World &world, const ShadowMap &shadow_map,
            ResourceManager &resource_manager, UiRenderer &ui_renderer)
{
    switch (world.status)
    {
    case WorldStatus_InitialLoad:
        main_render_loading(app, resource_manager);
        break;
    case WorldStatus_Paused:
        main_render_paused(app, ui_renderer, world.ui_state);
        break;
    case WorldStatus_Running:
        main_render_running(app, world, shadow_map, resource_manager);
        break;
    case WorldStatus_Finished:
        // Skip rendering if the game finished.
        break;
    default:
        LT_Panic("Unrecognized world state.");
    }
}

int
main()
{
#ifdef LT_DEBUG
    // Allow the program to crash if a nan value is computed.
    feenableexcept(FE_INVALID | FE_OVERFLOW);
#endif

    // --------------------------------------------------------------
    // TODO:
    //   1. Add frustum culling
    //   2. Reduce number of polygons needed to render the world!! (is it worth it?)
    // --------------------------------------------------------------
    Application app("Deferred renderer", 1680, 1050);
    IOTaskManager io_task_manager;

    ResourceManager resource_manager(&io_task_manager);
    {
        // TODO: Consider moving these raw strings into constants in a file, that way when
        // changing its value it is not necessary to rename it throughout the project but only
        // in one place.
        const char *shaders_to_load[] = {
            names::BASIC_SHADER,
            names::WIREFRAME_SHADER,
            names::SKYBOX_SHADER,
            names::FONT_SHADER,
            names::CROSSHAIR_SHADER,
            names::SHADOW_MAP_SHADER,
            names::SHADOW_MAP_RENDER_SHADER
        };
        const char *textures_to_load[] = {
            names::SKYBOX_TEXTURE, names::TEXTURES_16x16_TEXTURE
        };
        const char *fonts_to_load[] = {
            names::DEBUG_FONT,
            names::UI_FONT
        };

        for (auto name : shaders_to_load)
            resource_manager.load_from_shader_file(name);

        for (auto name : textures_to_load)
            resource_manager.load_from_texture_file(name);

        for (auto name : fonts_to_load)
            resource_manager.load_from_font_file(name);
    }

    Shader *wireframe_shader = resource_manager.get_shader(names::WIREFRAME_SHADER);
    wireframe_shader->load();
    wireframe_shader->setup_perspective_matrix(app.aspect_ratio());

    Shader *font_shader = resource_manager.get_shader(names::FONT_SHADER);
    font_shader->setup_orthographic_matrix(0, app.screen_width, app.screen_height, 0);

    Shader *skybox_shader = resource_manager.get_shader(names::SKYBOX_SHADER);
    skybox_shader->setup_perspective_matrix(app.aspect_ratio());

    AsciiFontAtlas *debug_font_atlas = resource_manager.get_font(names::DEBUG_FONT);
    LT_Assert(debug_font_atlas);
    debug_font_atlas->load(16.0f, 256, 256);

    AsciiFontAtlas *ui_font_atlas = resource_manager.get_font(names::UI_FONT);
    LT_Assert(ui_font_atlas);
    ui_font_atlas->load(54.0f, 1024, 1024);

    // NOTE, REFACTOR:
    // Call loading screen before creating the world instance, since it takes some time before
    // it is initialized. This looks ugly, maybe implement a loop only for the initial loading?
    // The world struct could also be initialized inside the loop, therefore, removing the need for this
    // function call.
    main_render_loading(app, resource_manager);

    const i32 seed = -1283;
    World world(app, seed, names::TEXTURES_16x16_TEXTURE, resource_manager, app.aspect_ratio());

    ShadowMap shadow_map(app.screen_width, app.screen_height,
                         names::SHADOW_MAP_SHADER, names::SHADOW_MAP_RENDER_SHADER, resource_manager);

    Shader *shadow_map_shader = resource_manager.get_shader(names::SHADOW_MAP_SHADER);
    shadow_map_shader->load();
    shadow_map_shader->use();
    shadow_map_shader->set_matrix("light_space", world.sun.light_space());

    Shader *basic_shader = resource_manager.get_shader(names::BASIC_SHADER);
    basic_shader->setup_perspective_matrix(app.aspect_ratio());
    basic_shader->use();
    basic_shader->set3f("sun.direction", world.sun.direction);
    basic_shader->set3f("sun.ambient", world.sun.ambient);
    basic_shader->set3f("sun.diffuse", world.sun.diffuse);
    basic_shader->set3f("sun.specular", world.sun.specular);
    basic_shader->set3f("sky_color", world.sky_color);
    basic_shader->set_matrix("light_space", world.sun.light_space());

    UiRenderer ui_renderer(names::FONT_SHADER, names::UI_FONT, resource_manager);

    //
    // Here starts the setup for the main loop
    //
    bool running = true;

    dump_opengl_errors("Before loop");

    // Define variables to control time
    using clock = std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    using std::chrono::nanoseconds;

    constexpr nanoseconds TIMESTEP(16ms); // Fixed timestep used for the updates
    nanoseconds lag(0ns);
    auto previous_time = clock::now();

    // Debug variables
    i32 num_updates = 0; // used for counting frames per second
    i32 num_frames = 0;  // used for counting updates per second
    auto start_second = clock::now(); // count seconds
    milliseconds min_frame_time(10000ms);
    milliseconds max_frame_time(0ms);

    World current_world = world;
    World previous_world = world;

    while (running)
    {
        // Update frame information.
        {
            auto current_time = clock::now();
            auto frame_time = current_time - previous_time;
            previous_time = current_time;
            lag += std::chrono::duration_cast<nanoseconds>(frame_time);

            if (frame_time > max_frame_time)
                max_frame_time = std::chrono::duration_cast<milliseconds>(frame_time);
            if (frame_time < min_frame_time)
                min_frame_time = std::chrono::duration_cast<milliseconds>(frame_time);
        }

        // Check if the window should close.
        if (app.should_close() || current_world.status == WorldStatus_Finished)
        {
            running = false;
            continue;
        }

        while (lag >= TIMESTEP)
        {
            glfwPollEvents();
            app.process_input();

            if (app.input.keys[GLFW_KEY_F5].was_pressed()) LT_Toggle(g_debug_context.render_shadow_map);
            if (app.input.keys[GLFW_KEY_F6].was_pressed()) LT_Toggle(g_debug_context.render_cascaded_frustum);
            if (app.input.keys[GLFW_KEY_T].was_pressed()) LT_Toggle(g_debug_context.render_wireframe);

            previous_world = current_world;
            current_world.update(app.input, shadow_map, basic_shader);

            num_updates++;
            lag -= TIMESTEP;
        }

        // Interpolate world state based on the current frame lag.
        const f32 lag_offset = (f32)lag.count() / TIMESTEP.count();
        World interpolated_world = World::interpolate(previous_world, current_world, lag_offset);

        // Render the interpolated state.
        main_render(app, interpolated_world, shadow_map, resource_manager, ui_renderer);
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
}
