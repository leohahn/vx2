#include "input.hpp"
#include <GLFW/glfw3.h>

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
