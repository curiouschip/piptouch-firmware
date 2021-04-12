#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

#define BTN_MODE    0x01
#define BTN_CAL     0x02

void buttons_init();
void buttons_get(uint8_t *out);

#endif
