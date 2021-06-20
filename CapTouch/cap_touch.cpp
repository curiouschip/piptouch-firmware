#include "cap_touch.h"

#include <Arduino.h>
#include <Wire.h>
#include "leds.h"

// AT42QT2120

#define ADDR 0x1C

#define REG_DETECTION_STATUS  2
#define REG_CALIBRATE         6
#define REG_RESET             7
#define REG_LP_MODE           8
#define REG_TTD               9
#define REG_ATD               10
#define REG_DI                11
#define REG_TRD               12
#define REG_DHT               13
#define REG_SLIDER            14
#define REG_CHARGE_TIME       15
#define REG_DTHR_BASE         16
#define REG_KEY_CTRL_BASE     28
#define REG_PULSE_SCALE_BASE  40
#define REG_MAX               99

#define SLIDER_KEY_START      0
#define SLIDER_KEY_COUNT      3

#define SLIDER_DEFAULT_DTHR   10
#define SLIDER_DEFAULT_PULSE  3
#define SLIDER_DEFAULT_SCALE  4

static uint8_t pad_to_key(uint8_t pad) {
  return 11 - pad;
}

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

static uint8_t reg_read(uint8_t reg) {
  Wire.beginTransmission(ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ADDR, 1);
  uint8_t val;
  Wire.readBytes(&val, 1);
  return val;
}

void cap_touch_init() {
	// Setup hardware reset pin
  PORTD |= (1 << 4);
	DDRD |= (1 << 4);
	
  cap_touch_reset();
}

void cap_touch_reset() {
  PORTD &= ~(1 << 4);
  delay(2);
  PORTD |= (1 << 4);
  delay(200);

  // reg_write(REG_RESET, 0xFF);
  // delay(200);

  // Enable slider, no wheel
  reg_write(REG_SLIDER, 0x80);

  // Write slider defaults
  for (int i = 0; i < SLIDER_KEY_COUNT; ++i) {
    reg_write(REG_DTHR_BASE + SLIDER_KEY_START + i, SLIDER_DEFAULT_DTHR);
    reg_write(REG_PULSE_SCALE_BASE + SLIDER_KEY_START + i, (SLIDER_DEFAULT_PULSE << 4) | SLIDER_DEFAULT_SCALE);
  }

  // Recalibrate
  cap_touch_recal(); 
}

void cap_touch_recal() {
  reg_write(REG_CALIBRATE, 0xFF);
  delay(2);
  while (reg_read(REG_DETECTION_STATUS) & 0x80) {
    /* spin */
  }
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

uint8_t cap_touch_read_reg(uint8_t reg) {
  if (reg > REG_MAX) {
    return 0xFF;
  }
  return reg_read(reg);
}

void cap_touch_write_reg(uint8_t reg, uint8_t val) {
  if (reg > REG_MAX) {
    return;
  }
  reg_write(reg, val);
}

void cap_touch_read_config(cap_touch_config_t *out) {
  out->lp_mode = reg_read(REG_LP_MODE);
  out->ttd = reg_read(REG_TTD);
  out->atd = reg_read(REG_ATD);
  out->detection_integrator = reg_read(REG_DI);
  out->touch_recal_delay = reg_read(REG_TRD);
  out->drift_hold_time = reg_read(REG_DHT);
  out->charge_time = reg_read(REG_CHARGE_TIME);
  out->slider = reg_read(REG_SLIDER);
  
  out->slider_detect_threshold = reg_read(REG_DTHR_BASE + SLIDER_KEY_START);
  out->slider_pulse_scale = reg_read(REG_PULSE_SCALE_BASE + SLIDER_KEY_START);

  for (int i = 0; i < CAP_TOUCH_PAD_COUNT; ++i) {
    uint8_t key = pad_to_key(i);
    out->key_detect_threshold[i] = reg_read(REG_DTHR_BASE + key);
    out->key_control[i] = reg_read(REG_KEY_CTRL_BASE + key);
    out->key_pulse_scale[i] = reg_read(REG_PULSE_SCALE_BASE + key);
  }
}

void cap_touch_write_config(cap_touch_config_t *in) {
  reg_write(REG_LP_MODE, in->lp_mode);
  reg_write(REG_TTD, in->ttd);
  reg_write(REG_ATD, in->atd);
  reg_write(REG_DI, in->detection_integrator);
  reg_write(REG_TRD, in->touch_recal_delay);
  reg_write(REG_DHT, in->drift_hold_time);
  reg_write(REG_CHARGE_TIME, in->charge_time);
  reg_write(REG_SLIDER, in->slider);

  for (int i = 0; i < SLIDER_KEY_COUNT; ++i) {
    reg_write(REG_DTHR_BASE + SLIDER_KEY_START + i, in->slider_detect_threshold);
    reg_write(REG_PULSE_SCALE_BASE + SLIDER_KEY_START + i, in->slider_pulse_scale);
  }
  
  for (int i = 0; i < CAP_TOUCH_PAD_COUNT; ++i) {
    uint8_t key = pad_to_key(i);
    reg_write(REG_DTHR_BASE + key, in->key_detect_threshold[i]);
    reg_write(REG_KEY_CTRL_BASE + key, in->key_control[i]);
    reg_write(REG_PULSE_SCALE_BASE + key, in->key_pulse_scale[i]);
  }
}
