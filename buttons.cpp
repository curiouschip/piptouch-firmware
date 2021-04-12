#include "buttons.h"

#include <Arduino.h>

#define BUTTON_COUNT  2
#define HOLD          3

static const uint8_t button_pins[] = { 5, 6 };
static const uint8_t button_flags[] = { BTN_CAL, BTN_MODE };
static uint8_t button_holds[] = { 0, 0 };
static uint8_t pressed = 0;

void buttons_init() {
  for (int i = 0; i < BUTTON_COUNT; ++i) {
    DDRB &= ~(1 << button_pins[i]);
  }
  
  TCCR3A = 0;
  TCCR3B = 0b00001100; // /256
  TCCR3C = 0;
  TCNT3 = 0;
  OCR3A = (F_CPU / 256) / 300; // 3.33ms
  TIMSK3 = 0b00000010;
}

void buttons_get(uint8_t *out) {
  TIMSK3 = 0;
  *out = pressed;
  pressed = 0;
  TIMSK3 = 0b00000010;
}

ISR(TIMER3_COMPA_vect) {
  for (int i = 0; i < BUTTON_COUNT; ++i) {
    if (!(PINB & (1 << button_pins[i]))) {
      if (button_holds[i] < HOLD) {
        if (++button_holds[i] == HOLD) {
          pressed |= button_flags[i];
        }
      }
    } else {
      button_holds[i] = 0;
    }
  }
}
