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
#include "io_task_manager.hpp"
#include "resource_manager.hpp"
#include "skybox.hpp"
#include "font.hpp"

lt_global_variable lt::Logger logger("main");
lt_global_variable Key g_keyboard[NUM_KEYBOARD_KEYS] = {};

// TODO: Remove this, since it is only for debugging.
lt_global_variable std::vector<Vertex_PU> g_text_buf;
lt_global_variable u32 atlas_id;

lt_internal void
main_render(const Application &app, const World &world, const Camera &camera, ResourceManager &resource_manager)
{
    Shader *basic_shader = resource_manager.get_shader("basic.glsl");
    Shader *wireframe_shader = resource_manager.get_shader("wireframe.glsl");
    Shader *font_shader = resource_manager.get_shader("font.glsl");
    Shader *deferred_shading_shader = resource_manager.get_shader("deferred_shading.glsl");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    app.bind_default_framebuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (world.state == WorldState_InitialLoad)
    {
        render_loading_screen();
        dump_opengl_errors("After loading screen", __FILE__);
    }
    else
    {
        app.bind_gbuffer();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (world.render_wireframe)
        {
            // Wireframe rendering
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            wireframe_shader->use();
            wireframe_shader->set_matrix("view", camera.view_matrix());
            render_world(world);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else
        {
            basic_shader->use();
            basic_shader->set_matrix("view", camera.view_matrix());
            render_world(world); // render world to the gbuffer
        }

        app.bind_default_framebuffer(); // render gbuffer back to the screen

        glUseProgram(deferred_shading_shader->program);
        deferred_shading_shader->set3f("view_position", camera.frustum.position);
        deferred_shading_shader->set1i("render_only_albedo", world.render_wireframe);

        render_final_quad(app, camera, deferred_shading_shader);

        // Draw the skybox

        // Copy the depth buffer from the gbuffer to the default framebuffer.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, app.gbuffer.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, app.screen_width, app.screen_height,
                          0, 0, app.screen_width, app.screen_height,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        world.skybox.shader->use();
        world.skybox.shader->set_matrix("view", camera.view_matrix());
        render_skybox(world.skybox);
        dump_opengl_errors("After render_skybox", __FILE__);
    }
    // FIXME: Remove this part
    {
        u32 vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex_PU)*g_text_buf.size(), &g_text_buf[0], GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_PU), (void*)offsetof(Vertex_PU, tex_coords));
        glEnableVertexAttribArray(1);

        font_shader->use();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glActiveTexture(GL_TEXTURE0 + font_shader->texture_unit("font_atlas"));
        glBindTexture(GL_TEXTURE_2D, atlas_id);

        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDrawArrays(GL_TRIANGLES, 0, g_text_buf.size());

        glDisable(GL_BLEND);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glBindVertexArray(0);
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
    //   - Create landscape with random noise.
    //   - Add decent main loop with interpolation.
    //   - Reduce number of polygons needed to render the world.
    // --------------------------------------------------------------

    IOTaskManager io_task_manager;

    ResourceManager resource_manager(&io_task_manager);
    {
        const char *shaders_to_load[] = {
            "basic.glsl",
            "deferred_shading.glsl",
            "wireframe.glsl",
            "skybox.glsl",
            "font.glsl",
        };
        const char *textures_to_load[] = {
            "skybox.texture",
        };

        for (auto name : shaders_to_load)
            resource_manager.load_from_shader_file(name);

        for (auto name : textures_to_load)
            resource_manager.load_from_texture_file(name);
    }

    Application app("Deferred renderer", 1024, 768);

    World world(resource_manager, app.aspect_ratio());
    // world.chunks[0][0][0].blocks[0][0][0] = BlockType_Terrain;
    world.chunks[0][0][0].blocks[2][0][0] = BlockType_Terrain;
    world.chunks[0][0][0].blocks[3][0][0] = BlockType_Terrain;

    Shader *basic_shader = resource_manager.get_shader("basic.glsl");
    basic_shader->load();
    basic_shader->setup_perspective_matrix(app.aspect_ratio());

    Shader *wireframe_shader = resource_manager.get_shader("wireframe.glsl");
    wireframe_shader->load();
    wireframe_shader->setup_perspective_matrix(app.aspect_ratio());

    Shader *font_shader = resource_manager.get_shader("font.glsl");
    font_shader->load();
    font_shader->setup_orthographic_matrix(0, app.screen_width, 0, app.screen_height);
    font_shader->add_texture("font_atlas");

    Shader *skybox_shader = resource_manager.get_shader("skybox.glsl");
    skybox_shader->load();
    skybox_shader->setup_perspective_matrix(app.aspect_ratio());
    skybox_shader->add_texture("texture_cubemap");

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

    bool running = true;

    dump_opengl_errors("Before loop", __FILE__);

    AsciiFontAtlas font_atlas(
        "/home/lhahn/dev/cpp/deferred-renderer/resources/fonts/dejavu/ttf/DejaVuSansMono.ttf",
        32.0f, 512, 512
    );

    LT_Assert(font_atlas.is_valid());

    g_text_buf = font_atlas.render_text_to_buffer("carro", 30.5f, 30.5f);
    atlas_id = font_atlas.id;

    for (auto &t : g_text_buf)
    {
        logger.log("position: ", t.position);
        logger.log("tex_coords: ", t.tex_coords);
        logger.log();
    }

    while (running)
    {
        process_input(app.window, g_keyboard);

        world.update(g_keyboard);

        // Check if the window should close.
        if (glfwWindowShouldClose(app.window))
        {
            running = false;
            continue;
        }

        main_render(app, world, world.camera, resource_manager);
    }

    io_task_manager.stop();
    io_task_manager.join_thread();
}
