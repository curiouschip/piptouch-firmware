#include "leds.h"

#include "Arduino.h"

void indicators_init() {
	PORTC &= ~(1 << 7);
	DDRC |= (1 << 7);
}

void indicators_set_ident(bool on) {
	if (on) {
		PORTC |= (1 << 7);
	} else {
		PORTC &= ~(1 << 7);
	}
}
