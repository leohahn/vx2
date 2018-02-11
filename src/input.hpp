#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include "lt_core.hpp"

#define NUM_KEYBOARD_KEYS 1024

struct GLFWwindow;

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

void process_input(GLFWwindow *win, Key *kb);

#endif // __INPUT_HPP__
