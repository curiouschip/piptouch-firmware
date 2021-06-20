#ifndef MODE_SELECTION_H
#define MODE_SELECTION_H

#include <stdint.h>

void mode_selection_init(int initial_mode);
void mode_selection_next();
int mode_selection_get();
bool mode_selection_set(int mode);
void mode_selection_emit(uint8_t buttons, int slider);

#endif