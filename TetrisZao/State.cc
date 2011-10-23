#include "Pch.h"
#include "State.h"

void change_state(state* next) {
	current_state->on_exit();
	current_state = next;
	current_state->on_enter();
}