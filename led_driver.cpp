#include "led_driver.h"
#include <Arduino.h>
#include <avr/interrupt.h>

#define TX_BUFFER_SIZE  (4 + (LED_COUNT * 4) + 4)

#define OFFSET_R        2
#define OFFSET_G        1
#define OFFSET_B        0

#define IRQ_OFF()       SPCR &= ~0x80
#define IRQ_ON()        SPCR |= 0x80

static byte input_buffer[LED_COUNT * 3] = { 0 };
static byte transmit_buffer[TX_BUFFER_SIZE];
static enum { TX_IDLE, TX_BUSY, TX_BUSY_DIRTY } tx_state = TX_IDLE;
static int tx_pos;

static void setup_transmit_buffer() {
  int wp = 0;
  transmit_buffer[wp++] = 0;
  transmit_buffer[wp++] = 0;
  transmit_buffer[wp++] = 0;
  transmit_buffer[wp++] = 0;
  for (int i = 0; i < LED_COUNT; ++i) {
    transmit_buffer[wp++] = 0xFF;
    transmit_buffer[wp++] = 0;
    transmit_buffer[wp++] = 0;
    transmit_buffer[wp++] = 0;
  }
  transmit_buffer[wp++] = 0xFF;
  transmit_buffer[wp++] = 0xFF;
  transmit_buffer[wp++] = 0xFF;
  transmit_buffer[wp++] = 0xFF;
}

static void copy_to_transmit_buffer() {
  int rp = 0;
  int wp = 5;
  for (int i = 0; i < LED_COUNT; ++i) {
    transmit_buffer[wp++] = input_buffer[rp++];
    transmit_buffer[wp++] = input_buffer[rp++];
    transmit_buffer[wp++] = input_buffer[rp++];
    wp++;
  }
}

static void transmit() {
  tx_state = TX_BUSY;
  tx_pos = 0;
  SPDR = transmit_buffer[tx_pos++];  
}

void leds_init() {
  setup_transmit_buffer();

  DDRB &= ~(1 << 3);
  DDRB |= (1 << 2) | (1 << 1) | (1 << 0); // set B0 as output so SPI master flag is not cleared
  SPCR = 0b11010001;
}

void leds_clear() {
  leds_set_all(0, 0, 0);
}

void leds_set(int offset, uint8_t r, uint8_t g, uint8_t b) {
  if (offset < 0 || offset >= LED_COUNT) {
    return;
  }
  int base = (LED_COUNT - 1 - offset) * 3;
  input_buffer[base+OFFSET_R] = r;
  input_buffer[base+OFFSET_G] = g;
  input_buffer[base+OFFSET_B] = b;
}

void leds_set_all(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_COUNT; ++i) {
    leds_set(i, r, g, b);
  }
}

void leds_flush() {
  IRQ_OFF();
  switch (tx_state) {
    case TX_IDLE:
      copy_to_transmit_buffer();
      transmit();
      break;
    case TX_BUSY:
      tx_state = TX_BUSY_DIRTY;
      break;
  }
  IRQ_ON();
}

ISR(SPI_STC_vect) {
  if (tx_pos == TX_BUFFER_SIZE) {
    if (tx_state == TX_BUSY_DIRTY) {
      transmit();
    } else {
      tx_state = TX_IDLE;
    }
  } else {
    SPDR = transmit_buffer[tx_pos++];
  }
}
