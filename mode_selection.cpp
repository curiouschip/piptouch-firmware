#include "mode_selection.h"
#include "mode_leds.h"
#include "modes.h"

static int current_mode = -1;

static bool is_valid_mode(int mode) {
	return (mode >= 1) && (mode <= MODE_COUNT);
}

static void activate(int mode) {
	if (current_mode >= 0) {
		Modes[current_mode]->deactivate();
	}
	current_mode = mode;
	Modes[current_mode]->activate();
	mode_leds_set_mode(current_mode);
}

void mode_selection_init(int initial_mode) {
	if (!is_valid_mode(initial_mode)) {
		initial_mode = 1;
	}
	activate(initial_mode - 1);
}

void mode_selection_next() {
	activate((current_mode + 1) % MODE_COUNT);
}

bool mode_selection_set(int new_mode) {
	if (!is_valid_mode(new_mode)) {
		return false;
	}
	activate(new_mode - 1);
	return true;
}

void mode_selection_emit(uint8_t buttons, int slider) {
	if (current_mode >= 0) {
		Modes[current_mode]->update(buttons, slider);	
	}
}