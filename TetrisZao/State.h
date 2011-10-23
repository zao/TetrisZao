#pragma once 

struct state {
	function<void ()> on_enter;
	function<void ()> on_exit;

	function<void (WPARAM)> on_key_down;
	function<void (WPARAM)> on_key_up;
	
	function<void (u32)> on_update;
};

void change_state(state* s);

extern state* current_state;