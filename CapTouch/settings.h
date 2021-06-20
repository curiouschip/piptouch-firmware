#ifndef SETTINGS_H
#define SETTINGS_H

#include "defaults.h"

// Settings
// --------
// Mode                 : int, range: 0..(MODE_COUNT-1)
// LED tracking enabled : bool
// MIDI channel         : uint8_t, range: 0..15
// MIDI controller      : uint8_t, range: 0..127

void settings_init(defaults_v1 *in);
void settings_export(defaults_v1 *out);

int settings_get_startup_mode();
bool settings_is_led_tracking_enabled();
uint8_t settings_get_midi_channel();
uint8_t settings_get_midi_controller();

void settings_set_startup_mode(int mode);
void settings_set_led_tracking_enabled(bool enabled);
void settings_set_midi(uint8_t ch, uint8_t ctl);

#endif
