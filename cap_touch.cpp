#include "cap_touch.h"

#include <Arduino.h>
#include <Wire.h>

#define ADDR 0x1C

static uint8_t reverse_byte(uint8_t c) {
  	uint8_t r = 0;
  	for (byte i = 0; i < 8; i++) {
    	r <<= 1;
    	r |= c & 1;
    	c >>= 1;
  	}
	return r;
}

static void reg_write(uint8_t reg, uint8_t val) {
	Wire.beginTransmission(ADDR);
	Wire.write(reg);
	Wire.write(val);
	Wire.endTransmission();
}

void cap_touch_init() {
	// Hardware reset
	DDRD |= (1 << 4);
	PORTD &= ~(1 << 4);
	delay(50);
	PORTD |= (1 << 4);

	reg_write(7, 0xFF); // reset
	delay(200);
	reg_write(14, 0x80); // enable slider
	reg_write(6, 0xFF); // calibrate
	delay(1000);

	reg_write(11, 4);

	const uint8_t dthr = 10;
	const uint8_t ps = (3 << 4) | 4;

	reg_write(16, dthr);
	reg_write(17, dthr);
	reg_write(18, dthr);

	reg_write(40, ps);
	reg_write(41, ps);
	reg_write(42, ps);
}

void cap_touch_update(cap_touch_state_t *state) {
	byte buf[3];

  	Wire.beginTransmission(ADDR);
  	Wire.write(2);
  	Wire.endTransmission(false);
  	Wire.requestFrom(ADDR, 3);
  	Wire.readBytes(buf, 3);
  	if (buf[0] & (1 << 1)) {
    	Wire.beginTransmission(ADDR);
    	Wire.write(5);
    	Wire.endTransmission(false);
    	Wire.requestFrom(0x1C, 1);
		uint8_t sv;
    	Wire.readBytes(&sv, 1);
    	state->slider = 255 - sv;
  	} else {
  		state->slider = -1;
  	}

  	state->buttons = (buf[1] >> 4) | (buf[2] << 4);
  	state->buttons = reverse_byte(state->buttons);
}