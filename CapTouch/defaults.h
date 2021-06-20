#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "Arduino.h"
#include "cap_touch.h"

struct __attribute__ ((packed)) defaults_v1 {
    uint8_t             startup_mode;
    uint8_t             led_tracking_enabled;
    uint8_t             midi_channel;
    uint8_t             midi_controller;
    cap_touch_config_t  ct;
};

#define DEFAULTS_LOADED   1
#define DEFAULTS_WRITTEN  2

bool defaults_init(defaults_v1 *values);
int defaults_save(const defaults_v1 *values);
void defaults_clear();

#endif
