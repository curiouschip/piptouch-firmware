#ifndef CAP_TOUCH_H
#define CAP_TOUCH_H

#include <stdint.h>

#define CAP_TOUCH_PAD_COUNT 8

typedef struct __attribute__ ((packed)) cap_touch_config {
  uint8_t lp_mode;
  uint8_t ttd;
  uint8_t atd;
  uint8_t detection_integrator;
  uint8_t touch_recal_delay;
  uint8_t drift_hold_time;
  uint8_t charge_time;

  uint8_t slider;
  uint8_t slider_detect_threshold;
  uint8_t slider_pulse_scale;

  // Order of these arrays is relative to board order, NOT captouch IC key number
  // e.g. index 0 => board pad 1
  uint8_t key_detect_threshold[CAP_TOUCH_PAD_COUNT];
  uint8_t key_control[CAP_TOUCH_PAD_COUNT];
  uint8_t key_pulse_scale[CAP_TOUCH_PAD_COUNT];
} cap_touch_config_t;

typedef struct cap_touch_state {
	uint8_t buttons;
	int slider;
} cap_touch_state_t;

void cap_touch_init();
void cap_touch_reset();
void cap_touch_recal();
void cap_touch_update(cap_touch_state_t *state);
uint8_t cap_touch_read_reg(uint8_t reg);
void cap_touch_write_reg(uint8_t reg, uint8_t val);
void cap_touch_read_config(cap_touch_config_t *out);
void cap_touch_write_config(cap_touch_config_t *in);

#endif
