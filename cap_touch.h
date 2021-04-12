#ifndef CAP_TOUCH_H
#define CAP_TOUCH_H

#include <stdint.h>

typedef struct cap_touch_state {
	uint8_t buttons;
	int slider;
} cap_touch_state_t;

void cap_touch_init();
void cap_touch_update(cap_touch_state_t *state);

#endif