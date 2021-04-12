#include "settings.h"

int Settings::initialMode = 1;
int Settings::serialReportMode = SERIAL_REPORT_MODE_BUTTON_STATES;
bool Settings::ledTracking = true;
uint8_t Settings::midiChannel = 0;
uint8_t Settings::midiController = 0;