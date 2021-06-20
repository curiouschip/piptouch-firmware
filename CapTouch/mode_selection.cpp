#include "mode_selection.h"
#include "modes.h"

static int current_mode = -1;

static void update_led() {
  	uint8_t out = PORTF & 0x0D;
  	if (current_mode == 0) {
  		out |= (1 << 1);
  	} else {
  		out |= (1 << (3 + current_mode));
  	}
  	PORTF = out;
}

static bool is_valid_mode(int mode) {
	return (mode >= 0) && (mode < MODE_COUNT);
}

static void activate(int mode) {
	if (current_mode >= 0) {
		Modes[current_mode]->deactivate();
	}
	current_mode = mode;
	update_led();
	Modes[current_mode]->activate();
}

void mode_selection_init(int initial_mode) {
	PORTF &= 0x0D;
	DDRF |= (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);

	if (!is_valid_mode(initial_mode)) {
		initial_mode = 0;
	}
	
	activate(initial_mode);
}

void mode_selection_next() {
	activate((current_mode + 1) % MODE_COUNT);
}

int mode_selection_get() {
	return current_mode;
}

bool mode_selection_set(int new_mode) {
	if (!is_valid_mode(new_mode)) {
		return false;
	}
	activate(new_mode);
	return true;
}

void mode_selection_emit(uint8_t buttons, int slider) {
	if (current_mode >= 0) {
		Modes[current_mode]->update(buttons, slider);	
	}
}
