#ifndef SETTINGS_H
#define SETTINGS_H

#include <EEPROM.h>

typedef enum {
  SERIAL_REPORT_MODE_BUTTON_STATES  = 1,
  SERIAL_REPORT_MODE_DEC            = 2,
  SERIAL_REPORT_MODE_HEX            = 3
} serial_report_mode_t;

class Settings {
public:
  static void init() {
    ledTracking = (EEPROM.read(0) != 0);
  }

  static void save() {
    EEPROM.write(0, ledTracking ? 1 : 0);
  }

  static int getInitialMode() { return initialMode; }
  static int getSerialReportMode() { return serialReportMode; }
  static bool isLEDTrackingEnabled() { return ledTracking; }
  static uint8_t getMIDIChannel() { return midiChannel; }
  static uint8_t getMIDIController() { return midiController; }

  static void setLEDTrackingEnabled(bool t) { ledTracking = t; }
  static void setMIDIChannel(uint8_t ch) { midiChannel = ch & 0x0F; }
  
private:
  static int initialMode;
  static int serialReportMode;
  static bool ledTracking;
  static uint8_t midiChannel;
  static uint8_t midiController;
};

#endif
