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
_dump_opengl_errors(const char *func, const char *file)
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
    // NOTE: Turning the vsync off does not seem to work on my machine.
    // So I don't know how realiable this setting is.
    glfwSwapInterval(0);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        LT_Panic("Failed to create glfw window.\n");
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

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
update_mouse_button_state(GLFWwindow *win, MouseState &mouse_state)
{
    const bool mouse_left_pressed = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT);

    if ((mouse_left_pressed && mouse_state.left_button_pressed) ||
        (!mouse_left_pressed && !mouse_state.left_button_pressed))
    {
        mouse_state.left_button_transition = Transition_None;
    }
    else if (mouse_left_pressed && !mouse_state.left_button_pressed)
    {
        mouse_state.left_button_pressed = true;
        mouse_state.left_button_transition = Transition_Down;
    }
    else if (!mouse_left_pressed && mouse_state.left_button_pressed)
    {
        mouse_state.left_button_pressed = false;
        mouse_state.left_button_transition = Transition_Up;
    }
}

lt_internal void
update_key_state(i32 key_code, Key *kb, GLFWwindow *win)
{
    const bool key_pressed = glfwGetKey(win, key_code);
    Key &key = kb[key_code];

    if ((key_pressed && key.is_pressed) || (!key_pressed && !key.is_pressed))
    {
        key.last_transition = Transition_None;
    }
    else if (key_pressed && !key.is_pressed)
    {
        key.is_pressed = true;
        key.last_transition = Transition_Down;
    }
    else if (!key_pressed && key.is_pressed)
    {
        key.is_pressed = false;
        key.last_transition = Transition_Up;
    }
}

void
Application::reset_mouse_position()
{
    const f64 xpos = screen_width/2.0;
    const f64 ypos = screen_height/2.0;
    input.mouse_state.prev_xpos = xpos;
    input.mouse_state.prev_ypos = ypos;
    glfwSetCursorPos(window, xpos, ypos);
}

bool
Application::should_close() const
{
    return glfwWindowShouldClose(window);
}

void
Application::process_input()
{
    {
        const i32 key_codes[] = {
            GLFW_KEY_ESCAPE, GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A,
            GLFW_KEY_D, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
            GLFW_KEY_ENTER,
            // Key codes used for debugging functionality.
            GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_T
        };

        for (auto key_code : key_codes)
            update_key_state(key_code, input.keys, window);
    }

    // Process mouse input
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    const i32 ixpos = floor(xpos), iypos = floor(ypos);
    input.mouse_state.xoffset = ixpos - input.mouse_state.prev_xpos;
    input.mouse_state.yoffset = iypos - input.mouse_state.prev_ypos;
    input.mouse_state.prev_xpos = ixpos;
    input.mouse_state.prev_ypos = iypos;

    // Check if the cursor is moving outside of the window boundaries.
    const i32 padding = 50;
    if (ixpos >= (screen_width-padding) || ixpos <= padding ||
        iypos >= (screen_height-padding) || iypos <= padding)
    {
        reset_mouse_position();
    }

    update_mouse_button_state(window, input.mouse_state);
}
