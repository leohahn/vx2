#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include "lt_core.hpp"

#define NUM_KEYBOARD_KEYS 1024

struct Application;

struct Key
{
	enum Transition
	{
		Transition_None = 0,
		Transition_Up = 1,
		Transition_Down = 2,
	};

	bool is_pressed;
	Transition last_transition;
};

struct MouseState
{
    f64 xoffset;
    f64 yoffset;
    f64 prev_xpos;
    f64 prev_ypos;

    MouseState(i32 screen_width, i32 screen_height)
        : xoffset(0.0)
        , yoffset(0.0)
        , prev_xpos(screen_width/2.0)
        , prev_ypos(screen_height/2.0)
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
