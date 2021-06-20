#include "defaults.h"

#include <avr/eeprom.h>

static int get_version() {
  uint8_t b1 = eeprom_read_byte(0);
  uint8_t b2 = eeprom_read_byte(1);
  uint8_t b3 = eeprom_read_byte(2);
  uint8_t version = eeprom_read_byte(3);

  if (b1 != 'P' || b2 != '/' || b3 != 'T') {
    return -1;
  }

  return version;
}

static int write_defaults(uint8_t version, void *data, int len) {
  eeprom_update_block(data, 4, len);
  eeprom_update_byte(0, 'P');
  eeprom_update_byte(1, '/');
  eeprom_update_byte(2, 'T');
  eeprom_update_byte(3, version);
}

bool defaults_init(defaults_v1 *values) {
  int version = get_version();
  if (version == 1) {
    eeprom_read_block(values, 4, sizeof(defaults_v1));
    return true;
  } else {
    return false;
  }
}

int defaults_save(const defaults_v1 *values) {
  write_defaults(1, values, sizeof(defaults_v1));
  return 0;
}

void defaults_clear() {
  eeprom_update_byte(0, 0xFF);
  eeprom_update_byte(1, 0xFF);
  eeprom_update_byte(2, 0xFF);
  eeprom_update_byte(3, 0xFF);
}
