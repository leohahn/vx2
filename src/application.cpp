#include "application.hpp"
#include <cstring>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "lt_utils.hpp"
#include "unit_plane_data.hpp"

lt_global_variable lt::Logger logger("application");
lt_global_variable lt::Logger gl_logger("OpenGL");

lt_internal void
framebuffer_size_callback(GLFWwindow *w, i32 width, i32 height)
{
    LT_Unused(w);
    glViewport(0, 0, width, height);
}

void
dump_opengl_errors(const char *func, const char *file)
{
#ifdef LT_DEBUG
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        lt_local_persist char desc[50] = {};
        switch (err)
        {
        case GL_INVALID_ENUM: {
            const char *s = "Invalid enumeration.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_INVALID_VALUE: {
            const char *s = "Invalid value.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_INVALID_OPERATION: {
            const char *s = "Invalid operation.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_INVALID_FRAMEBUFFER_OPERATION: {
            const char *s = "Invalid framebuffer operation.";
            std::sprintf(desc, "%s", s);
            break;
        }
        case GL_OUT_OF_MEMORY: {
            const char *s = "Out of memory";
            std::sprintf(desc, "%s", s);
            break;
        }
        default: LT_Assert(false);
        }
        if (file)
            gl_logger.log(func, " (", file, ") : ", desc);
        else
            gl_logger.log(func,": ", desc);
    }
#endif
}

Application::~Application()
{
    logger.log("Releasing application resources.");
    // free mesh resources
    glDeleteVertexArrays(1, &render_quad.vao);
    glDeleteBuffers(1, &render_quad.vbo);
    glDeleteBuffers(1, &render_quad.ebo);
    // free GLFW resources
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::bind_default_framebuffer() const {glBindFramebuffer(GL_FRAMEBUFFER, 0);}

Application::Application(const char *title, i32 width, i32 height)
    : title(title)
    , screen_width(width)
    , screen_height(height)
    , input(width, height)

{
    logger.log("Creating the application.");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // This does not work however for me.
    glfwSwapInterval(0); // Set vsync off

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        LT_Panic("Failed to create glfw window.\n");
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        LT_Panic("Failed to initialize GLAD\n");

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glViewport(0, 0, width, height);
}

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

void
Application::process_input()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    const i32 key_codes[] = {
        GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_UP,
        GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_T,
    };

    for (auto key_code : key_codes)
        update_key_state(key_code, input.keys, window);

    // Process mouse input
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    input.mouse_state.xoffset = xpos - input.mouse_state.prev_xpos;
    input.mouse_state.yoffset = ypos - input.mouse_state.prev_ypos;
    input.mouse_state.prev_xpos = xpos;
    input.mouse_state.prev_ypos = ypos;
}
