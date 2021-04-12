#include "mode_leds.h"

#include <Arduino.h>

void mode_leds_init() {
	PORTF &= 0x0D;
	DDRF |= (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);
}

void mode_leds_startup_sequence() {
	//  int seq[] = {
  	//   0, 1, 2, 3, 2, 1
  	// };

  	// int e = 0, l = 0, d = 25;
  	// while (e < 2600) {
  	//   PORTF = 1 << (4 + seq[l]);
  	//   delay(d);
  	//   e += d;
  	//   d += 7;
  	//   l = (l + 1) % 6;
  	// }
}

void mode_leds_set_mode(int mode) {
  uint8_t out = PORTF & 0x0D;
  
  if (mode == 0) {
    out |= (1 << 1);
  } else {
    out |= (1 << (3 + mode));
  }
  
	PORTF = out;
}
