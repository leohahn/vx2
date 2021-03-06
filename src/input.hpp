#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include "lt_core.hpp"
#include <cmath>

#define NUM_KEYBOARD_KEYS 1024

struct Application;

enum Transition
{
		Transition_None = 0,
		Transition_Up = 1,
		Transition_Down = 2,
};

struct Key
{
    bool is_pressed;
    Transition last_transition;

    inline bool was_pressed() const { return last_transition == Transition_Down; }
};

struct MouseState
{
    i32 xoffset;
    i32 yoffset;
    i32 prev_xpos;
    i32 prev_ypos;

    Transition left_button_transition;
    bool left_button_pressed;

    MouseState(i32 screen_width, i32 screen_height)
        : xoffset(0)
        , yoffset(0)
        , prev_xpos(floor(screen_width/2.0))
        , prev_ypos(floor(screen_height/2.0))
    {}
};

struct Input
{
    Key keys[NUM_KEYBOARD_KEYS] = {};
    MouseState mouse_state;

    Input(i32 screen_width, i32 screen_height)
        : mouse_state(screen_width, screen_height)
    {}
};

void process_input(Application &app, Key *kb);

#endif // __INPUT_HPP__
