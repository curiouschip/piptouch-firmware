#include "led_driver.h"
#include "console.h"
#include "buttons.h"
#include "settings.h"
#include "cap_touch.h"
#include "mode_leds.h"
#include "mode_selection.h"

#include <Wire.h>

void setup() {  
  Wire.begin();

  mode_leds_init();

  leds_init();
  leds_flush();

  mode_leds_startup_sequence();

  Settings::init();

  cap_touch_init();
  mode_selection_init(Settings::getInitialMode());
  console_init();
}

void loop() {
//  DDRB |= (1 << 0);
//  DDRC |= (1 << 7);
//  DDRD |= (1 << 5);
//  PORTB |= (1 << 0);
//  PORTC |= (1 << 7);
//  PORTD |= (1 << 5);
  
  uint8_t btn_state = 0;
  buttons_get(&btn_state);
  
  if (btn_state & BTN_MODE) {
    mode_selection_next();
  }

  if (btn_state & BTN_CAL) {
    // TODO: calibrate  
  }

  console_tick();
  cap_touch_state_t cs;
  cap_touch_update(&cs);
  mode_selection_emit(cs.buttons, cs.slider);
  updateTrackingLEDs(cs.buttons, cs.slider);
}

void updateTrackingLEDs(uint8_t buttons, int slider) {
  if (!Settings::isLEDTrackingEnabled()) {
    return;
  }
  int threshold = 0;
  for (int i = 0; i < 8; ++i) {
    leds_set(i, (buttons & 0x01) ? 15 : 0, (slider > threshold) ? 15 : 0, 0);
    buttons >>= 1;
    threshold += 32;
  }
  leds_flush();    
}
