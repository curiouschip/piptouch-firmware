#include "console.h"

#include "Arduino.h"
#include "settings.h"
#include "cap_touch.h"
#include "led_driver.h"
#include "leds.h"
#include "mode_selection.h"
#include "modes.h"
#include "version.h"

#include <string.h>

#define IS_NL(ch)     ((ch) == '\r' || (ch) == '\n')
#define EQ(str, val)  (strcmp((str), (val)) == 0)

#define OK             0
#define ESIZE         -1
#define EFORMAT       -2
#define EUSAGE        -3
#define EARG          -4

static char cmd[CONSOLE_CMD_BUFFER_SIZE];
static enum { RX_MSG, RX_NL, RX_OVERRUN } rx_state = RX_MSG;
static int rx_pos = 0;
static bool quiet;

static int doClearSettings(char*);
static int doCTConfig(char*);
static int doCTRecal(char*);
static int doCTReg(char*);
static int doCTReset(char*);
static int doHello(char*);
static int doIdent(char*);
static int doLED(char*);
static int doMIDI(char*);
static int doMode(char*);
static int doSave(char*);
static int doTrack(char*);

struct command_handler {
  const char *op;
  int (*handler)(char *first_arg);
};

static const command_handler handlers[] = {
  { "clear_settings", doClearSettings },
  { "ct_config",      doCTConfig      },
  { "ct_recal",       doCTRecal       },
  { "ct_reg",         doCTReg         },
  { "ct_reset",       doCTReset       },
  { "hello",          doHello         },
  { "ident",          doIdent         },
  { "led",            doLED           },
  { "midi",           doMIDI          },
  { "mode",           doMode          },
  { "save",           doSave          },
  { "track",          doTrack         },
  { NULL,             NULL            }
};

static const char usage_clear_settings[] PROGMEM =
  "clear_settings: clears saved settings from EEPROM";

static const char usage_ct_config[] PROGMEM =
  "ct_config         : read cap touch config\r\n"
  "ct_config <config>: write cap touch config\r\n"
  "\r\n"
  "<config>: 34 byte config string (hex-encoded)";  

static const char usage_ct_recal[] PROGMEM =
  "ct_recal: recalibrate the cap touch IC";

static const char usage_ct_reg[] PROGMEM =
  "ct_reg <reg>      : read cap touch register\r\n"
  "ct_reg <reg> <val>: write cap touch register\r\n"
  "\r\n"
  "<reg>: register (0-99)\r\n"
  "<val>: value (0-255)";

static const char usage_ct_reset[] PROGMEM =
  "ct_reset: reset the cap touch IC";

static const char usage_hello[] PROGMEM =
  "hello: get product name and version";

static const char usage_ident[] PROGMEM =
  "ident [on | off]: turn ident LED on/off";

static const char usage_led[] PROGMEM =
  "led off            : turn off all RGB LEDs\r\n"
  "led <color>        : set all LEDs to <color>\r\n"
  "led <range> <color>: set LEDs included in <range> to <color>\r\n"
  "\r\n"
  "Colors are expressed as HTML hex values e.g. #ff0000";

static const char usage_midi[] PROGMEM =
  "midi                       : get MIDI config\r\n"
  "midi <channel> <controller>: set MIDI config\r\n"
  "\r\n"
  "<channel>   : MIDI channel (1-16)\r\n"
  "<controller>: MIDI controller (0-127, 0 = pitch bend)";

static const char usage_mode[] PROGMEM =
  "mode       : get active report mode\r\n"
  "mode <mode>: set report mode\r\n"
  "\r\n"
  "<mode>: one of serial, numeric, cursor, gamepad, midi";

static const char usage_save[] PROGMEM =
  "save: save active settings to EEPROM as the power-on defaults";

static const char usage_track[] PROGMEM =
  "track           : get LED tracking status\r\n"
  "track [on | off]: set LED tracking status\r\n"
  "\r\n"
  "When LED tracking is enabled the LEDs synchronise with the state of the\r\n"
  "cap touch keys/slider";

static const char *const usage_strings[] PROGMEM = {
  usage_clear_settings,
  usage_ct_config,
  usage_ct_recal,
  usage_ct_reg,
  usage_ct_reset,
  usage_hello,
  usage_ident,
  usage_led,
  usage_midi,
  usage_mode,
  usage_save,
  usage_track
};

static const char* mode_names[] = {
  "serial",
  "numeric",
  "cursor",
  "midi",
  "gamepad"
};

//
// Helpers

static inline char* next_arg() {
  return strtok(NULL, " ");
}

static int ok() {
  if (!quiet) {
    CONSOLE_PORT.println(F("OK"));  
  }
  return OK;
}

static void print_hex_byte(uint8_t val) {
  if (quiet) {
    return;
  }
  if (val < 0x10) {
    CONSOLE_PORT.print("0");
  }
  CONSOLE_PORT.print(val, HEX);
}

//
// Value parsers

static byte parseHexit(char v) {
  if (v >= '0' && v <= '9') return v - '0';
  if (v >= 'a' && v <= 'f') return v - 'a' + 10;
  return 255;
}

static int parseHex(uint8_t *dst, const char *src, int len) {
  if (strlen(src) != (len*2)) {
    return ESIZE;
  }

  for (int i = 0; i < len; ++i) {
    uint8_t hi = parseHexit(*(src++));
    uint8_t lo = parseHexit(*(src++));
    if (hi == 255 || lo == 255) {
      return EFORMAT;
    }
    dst[i] = (hi << 4) | lo;
  }

  return OK;
}

static bool parseBool(char *v, bool *out) {
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

static bool parseColor(char *str, uint8_t *r, uint8_t *g, uint8_t *b) {
  uint8_t rgb[3];
  if (str[0] != '#' || parseHex(rgb, str+1, 3) < 0) {
    return false;
  }
  *r = rgb[0];
  *g = rgb[1];
  *b = rgb[2];
  return true;
}

static bool parseInt(char *str, int *v) {
  int out = 0;
  bool negative = false;
  
  if (str[0] == '0' && str[1] == 'x') {
    str += 2;
    while (*str) {
      uint8_t n = parseHexit(*(str++));
      if (n == 255) {
        return false;
      }
      out = (out << 4) | n;
    }
    *v = out;
    return true;
  }

  if (*str == '-') {
    negative = true;
    str++;
  }

  while (*str) {
    char ch = *(str++);
    if (ch < '0' || ch > '9') {
      return false;
    }
    out = (out * 10) + (ch - '0');
  }

  *v = negative ? (-out) : out;

  return true;
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

//
//

static void usage(int cmd_ix) {
  CONSOLE_PORT.print(F("Usage:\r\n  "));
  uint16_t string_addr = pgm_read_word(&(usage_strings[cmd_ix]));
  while (true) {
    uint8_t b = pgm_read_byte(string_addr);
    if (!b) {
      break;
    }
    CONSOLE_PORT.write(b);
    if (b == '\n') {
      CONSOLE_PORT.print(F("  "));
    }
    string_addr++;
  }
  CONSOLE_PORT.println("");
  CONSOLE_PORT.println("");
}

static void dispatch() {
  for(int i = 0; cmd[i]; i++){
    cmd[i] = tolower(cmd[i]);
  }

  char *op = strtok(cmd, " ");
  if (!op) {
    return;
  }

  char *first_arg = next_arg();
  
  quiet = false;
  if (op[0] == '@') {
    quiet = true;
    op++;
  }

  for (int i = 0; handlers[i].op != NULL; ++i) {
    if (EQ(op, handlers[i].op)) {
      if (first_arg != NULL && EQ(first_arg, "help")) {
        if (!quiet) {
          usage(i);  
        }
        return;
      }
      
      int ret = handlers[i].handler(first_arg);
      if (!quiet) {
        switch (ret) {
          case EUSAGE:
            usage(i);
            break;
          case EARG:
            CONSOLE_PORT.print(F("Error: invalid argument(s) - type '"));
            CONSOLE_PORT.print(op);
            CONSOLE_PORT.println(F(" help' for instructions"));
            break;
        }  
      }

      return;
    }
  }

  if (!quiet) {
    CONSOLE_PORT.print(F("Error: unknown command '"));
    CONSOLE_PORT.print(op);
    CONSOLE_PORT.println("'");
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

//
// Command handlers

static int doClearSettings(char *ignore) {
  defaults_clear();
  return ok();
}

static int doCTConfig(char *blob) {
  cap_touch_config_t cfg;

  if (!blob) {
    if (!quiet) {
      cap_touch_read_config(&cfg);
      CONSOLE_PORT.print(F("ct_config: "));
      uint8_t *bytes = (uint8_t*)&cfg;
      for (int i = 0; i < sizeof(cap_touch_config_t); ++i) {
        print_hex_byte(bytes[i]);
      }
      CONSOLE_PORT.println("");  
    }
    return OK;
  }

  int err = parseHex((uint8_t*)&cfg, blob, sizeof(cap_touch_config_t));
  if (err < 0) {
    return EARG;
  }

  cap_touch_write_config(&cfg);
  return ok();
}

static int doCTRecal(char *ignore) {
  cap_touch_recal();
  return ok();
}

static int doCTReg(char *regstr) {
  if (!regstr)
    return EUSAGE;

  int reg;
  if (!parseInt(regstr, &reg)) {
    return EARG;
  }

  if (reg < 0 || reg > 99) {
    return EARG;
  }

  char *valstr = next_arg();
  if (!valstr) {
    if (!quiet) {
      CONSOLE_PORT.print(F("ct_reg: 0x"));
      print_hex_byte(cap_touch_read_reg(reg));
      CONSOLE_PORT.println("");  
    }
    return OK;
  }

  int val;
  if (!parseInt(valstr, &val)) {
    return EARG;
  }

  if (val < 0 || val > 255) {
    return EARG;
  }

  cap_touch_write_reg(reg, val);
  return ok();
}

static int doCTReset(char *ignore) {
  cap_touch_reset();
  return ok();
}

static int doHello(char *ignore) {
  if (!quiet) {
    CONSOLE_PORT.println(F("hello! PipTouch (hw=" PT_HW_VERSION_STR ";fw=" PT_FW_VERSION_STR ")"));  
  }
  return OK;
}

static int doIdent(char *val) {
  bool on;

  if (!val)
    return EUSAGE;

  if (!parseBool(val, &on))
    return EARG;

  indicators_set_ident(on);
  return ok();
}

static int doLED(char *range) {
  char *color;
  uint8_t mask, r, g, b;

  if (!range)
    return EUSAGE;

  if (EQ(range, "off")) {
    leds_set_all(0, 0, 0);
    range = next_arg();
  } else if (range[0] == '#') {
    if (parseColor(range, &r, &g, &b)) {
      leds_set_all(r, g, b);  
      range = next_arg();
    } else {
      return EARG;
    }
  }

  while (range) {
    color = next_arg();
    if (!color) {
      return EUSAGE;
    }
    
    if (!parseLEDMask(range, &mask)) {
      return EARG;
    }
    
    if (!parseColor(color, &r, &g, &b)) {
      return EARG;
    }
    
    int led = 0;
    while (mask) {
      if (mask & 1) {
        leds_set(led, r, g, b);
      }
      led++;
      mask >>= 1;
    }

    range = next_arg();
  }

  if (settings_is_led_tracking_enabled()) {
    settings_set_led_tracking_enabled(false);
    // There's a bug in the LED driver but I don't have the energy to fix it right now.
    // Giving any pending update time to flush appears to be a reliable workaround.
    delay(2);
  }

  leds_flush();
  return ok();
}

static int doMIDI(char *str_channel) {
  if (!str_channel) {
    if (!quiet) {
      CONSOLE_PORT.print(F("midi: "));
      CONSOLE_PORT.print(settings_get_midi_channel()+1);
      CONSOLE_PORT.print(" ");
      CONSOLE_PORT.println(settings_get_midi_controller());  
    }
    return OK;
  }

  char *str_controller = next_arg();
  if (!str_controller)
    return EUSAGE;

  int channel, controller;
  if (!parseInt(str_channel, &channel) || !parseInt(str_controller, &controller))
    return EARG;

  if (channel < 1 || channel > 16 || controller < 0 || controller > 127)
    return EARG;

  settings_set_midi(channel - 1, controller);
    
  return ok();
}

static int doMode(char *mode) {
  if (!mode) {
    if (!quiet) {
      CONSOLE_PORT.print(F("mode: "));
      CONSOLE_PORT.println(mode_names[mode_selection_get()]);
    }
    return OK;
  }

  for (int i = 0; i < MODE_COUNT; ++i) {
    if (EQ(mode_names[i], mode)) {
      mode_selection_set(i);
      return ok();
    }
  }

  return EARG;
}

static int doSave(char *ignore) {
  defaults_v1 to_save;
  settings_export(&to_save);
  cap_touch_read_config(&to_save.ct);
  to_save.startup_mode = mode_selection_get();
  defaults_save(&to_save);
  return ok();
}

static int doTrack(char *val) {
  bool on;

  if (!val) {
    if (!quiet) {
      CONSOLE_PORT.print(F("track: "));
      CONSOLE_PORT.println(settings_is_led_tracking_enabled() ? "on" : "off");  
    }
    return OK;
  }

  if (!parseBool(val, &on))
    return EARG;
  
  settings_set_led_tracking_enabled(on);
  leds_clear();

  return ok();
}
