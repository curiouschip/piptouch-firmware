#include "led_driver.h"
#include "console.h"
#include "buttons.h"
#include "defaults.h"
#include "settings.h"
#include "cap_touch.h"
#include "mode_selection.h"
#include "leds.h"

#include <Wire.h>

void setup_defaults() {
  defaults_v1 defaults;
  defaults.startup_mode = 0;
  defaults.led_tracking_enabled = 1;
  defaults.midi_channel = 0;
  defaults.midi_controller = 0;

  if (defaults_init(&defaults)) {
    cap_touch_write_config(&defaults.ct);
  } 

  settings_init(&defaults);
}

void setup() {
  Wire.begin();

  indicators_init();
  buttons_init();
  leds_init();
  leds_flush();
  cap_touch_init();

  setup_defaults();
  
  console_init();
  mode_selection_init(settings_get_startup_mode());

  interrupts();
}

void loop() {
  uint8_t btn_state;
  buttons_get(&btn_state);
  
  if (btn_state & BTN_MODE) {
    mode_selection_next();
  }

  if (btn_state & BTN_CAL) {
    indicators_set_ident(true);
    // cap_touch_recal();
    delay(50);
    indicators_set_ident(false);
  }

  console_tick();
  
  cap_touch_state_t cs;
  cap_touch_update(&cs);
  mode_selection_emit(cs.buttons, cs.slider);
  update_tracking_leds(cs.buttons, cs.slider);
}

void update_tracking_leds(uint8_t buttons, int slider) {
  if (!settings_is_led_tracking_enabled()) {
    return;
  }
  int threshold = 0;
  for (int i = 0; i < 8; ++i) {
    leds_set(i, (buttons & 0x01) ? 8 : 0, (slider > threshold) ? 8 : 0, 0);
    buttons >>= 1;
    threshold += 32;
  }
  leds_flush();    
}
