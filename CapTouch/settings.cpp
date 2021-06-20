#include "settings.h"

#include "Arduino.h"
#include "modes.h"

static int mode;
static bool tracking;
static uint8_t midi_ch, midi_ctl;

void settings_init(defaults_v1 *in) {
  settings_set_startup_mode(in->startup_mode);
  settings_set_led_tracking_enabled(in->led_tracking_enabled > 0);
  settings_set_midi(in->midi_channel, in->midi_controller);
}

void settings_export(defaults_v1 *out) {
  out->startup_mode = mode;
  out->led_tracking_enabled = tracking;
  out->midi_channel = midi_ch;
  out->midi_controller = midi_ctl;  
}

int settings_get_startup_mode() { return mode; }
bool settings_is_led_tracking_enabled() { return tracking; }
uint8_t settings_get_midi_channel() { return midi_ch; }
uint8_t settings_get_midi_controller() { return midi_ctl; }

void settings_set_startup_mode(int new_mode) {
  if (new_mode < 0) {
    new_mode = 0;
  } else if (new_mode >= MODE_COUNT) {
    new_mode = MODE_COUNT-1;
  }
  mode = new_mode;
}

void settings_set_led_tracking_enabled(bool enabled) {
  tracking = enabled;
}

void settings_set_midi(uint8_t ch, uint8_t ctl) {
  midi_ch = ch & 0x0F;
  midi_ctl = ctl & 0x7F;
}
