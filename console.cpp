#include "console.h"

#include "Arduino.h"
#include "settings.h"
#include "led_driver.h"
#include "mode_selection.h"
#include "modes.h"

#include <string.h>

#define IS_NL(ch)     ((ch) == '\r' || (ch) == '\n')
#define EQ(str, val)  (strcmp((str), (val)) == 0)

static char cmd[CONSOLE_CMD_BUFFER_SIZE];
static enum { RX_MSG, RX_NL, RX_OVERRUN } rx_state = RX_MSG;
static int rx_pos = 0;
static bool echo = false;

static bool parseBool(bool *out) {
  char *v = strtok(NULL, " ");
  if (v) {
    if (EQ(v, "on") || EQ(v, "true") || EQ(v, "yes")) {
      *out = true;
      return true; 
    } else if (EQ(v, "off") || EQ(v, "false") || EQ(v, "no")) {
      *out = false;
      return true;      
    }
  }
  return false;
}

static bool parseLEDMask(char *str, uint8_t *out) {
  uint8_t mask = 0;
  uint8_t v1, v2;
  int state = 0;
  while (*str) {
    switch (state) {
      case 0: // reading first number
      {
        if (!isdigit(*str)) {
          return false;
        }
        v1 = *str - '0';
        if (v1 >= LED_COUNT) {
          return false;
        }
        state = 1;
        break;
      }
      case 1: // got first number
      {
        if (*str == ',') {
          mask |= (1 << v1);
          state = 0;
        } else if (*str == '-') {
          state = 2;
        } else {
          return false;
        }
        break;
      }
      case 2: // range
      {
        if (!isdigit(*str)) {
          return false;
        }
        v2 = *str - '0';
        if (v2 >= LED_COUNT) {
          return false;
        }
        for (uint8_t i = v1; i <= v2; ++i) {
          mask |= (1 << i);
        }
        state = 3;
        break;
      }
      case 3: // range complete
      {
        if (*str == ',') {
          state = 0;
        } else {
          return false;
        }
        break;
      }
    }
    str++;
  }

  if (state == 1) {
    mask |= (1 << v1);
  }

  *out = mask;
  
  return true;
}

static byte parseHexit(char v) {
  if (v >= '0' && v <= '9') return v - '0';
  if (v >= 'a' && v <= 'f') return v - 'a' + 10;
  return 255;
}

static bool parseColorComponent(char *str, uint8_t *v) {
  byte hi = parseHexit(str[0]);
  byte lo = parseHexit(str[1]);
  if (lo == 255 || hi == 255) {
    return false;
  }
  *v = (hi << 4) | lo;
  return true;
}

static bool parseColor(char *str, uint8_t *r, uint8_t *g, uint8_t *b) {
  return (strlen(str) == 7)
      && str[0] == '#'
      && parseColorComponent(str+1, r)
      && parseColorComponent(str+3, g)
      && parseColorComponent(str+5, b);
}

static void doTrack() {
  bool on;
  if (!parseBool(&on)) {
    CONSOLE_PORT.println("Usage: track <bool>");
    return;
  }
  Settings::setLEDTrackingEnabled(on);
  if (!on) {
    leds_clear();
  }
}

static void doLED() {
  int ix = 0;
  while (true) {
    char *range, *color;
    uint8_t mask, r, g, b;
    
    range = strtok(NULL, " ");
    if (!range) {
      break;
    }

    // 
    if (ix == 0) {
      if (EQ(range, "off")) {
        leds_set_all(0, 0, 0);
        continue;
      } else if (range[0] == '#') {
        if (parseColor(range, &r, &g, &b)) {
          leds_set_all(r, g, b);  
          continue;
        } else {
          CONSOLE_PORT.println("Invalid color");
          return;
        }
      }
    }
    
    color = strtok(NULL, " ");
    if (!color) {
      goto usage;
    }
    
    if (!parseLEDMask(range, &mask)) {
      CONSOLE_PORT.println("Invalid range");
      return;
    }
    
    if (!parseColor(color, &r, &g, &b)) {
      CONSOLE_PORT.println("Invalid color");
      return;
    }
    
    int led = 0;
    while (mask) {
      if (mask & 1) {
        leds_set(led, r, g, b);
      }
      led++;
      mask >>= 1;
    }

    ix++;
  }

  Settings::setLEDTrackingEnabled(false);
  
  leds_flush();
  return;

usage:
  CONSOLE_PORT.println("Usage: led <range> <hex-color>");
}

static void doMode() {
  char *mode = strtok(NULL, " ");
  if (!mode) {
    CONSOLE_PORT.println("Usage: mode <mode>");
    return;
  }

  int modeNum = String(mode).toInt();
  if (!mode_selection_set(modeNum)) {
    CONSOLE_PORT.println("Invalid mode");
  }
}

static void dispatch() {
  for(int i = 0; cmd[i]; i++){
    cmd[i] = tolower(cmd[i]);
  }

  char *op = strtok(cmd, " ");
  if (!op) {
    return;
  }

  if (EQ(op, "hello")) {
    CONSOLE_PORT.println("CapTouch v0.1");
  } else if (EQ(op, "track")) {
    doTrack(); 
  } else if (EQ(op, "save")) {
    Settings::save();
    CONSOLE_PORT.println("Settings saved");
  } else if (EQ(op, "led")) {
    doLED();
  } else if (EQ(op, "mode")) {
    doMode();
  }
}

void console_init() {
  CONSOLE_PORT.begin(CONSOLE_BAUD_RATE);
}

void console_tick() {
  while (Serial.available()) {
    char ch = CONSOLE_PORT.read();
    switch (rx_state) {
      case RX_MSG:
        if (IS_NL(ch)) {
          if (rx_pos > 0) {
            cmd[rx_pos] = '\0';
            dispatch();
            rx_state = RX_NL;
          }
        } else {
          cmd[rx_pos++] = ch;
          if (rx_pos == CONSOLE_CMD_BUFFER_SIZE) {
            rx_state = RX_OVERRUN;
          }
        }
        break;
      case RX_NL:
        if (!IS_NL(ch)) {
          cmd[0] = ch;
          rx_pos = 1;
          rx_state = RX_MSG;
        }
        break;
      case RX_OVERRUN:
        if (IS_NL(ch)) {
          rx_state = RX_NL;
        }
        break;
    }
  }
}
