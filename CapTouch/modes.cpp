#include "modes.h"

static SerialMode serial;
static NumericKeyboardMode keys1;
static CursorKeyboardMode keys2;
static MIDIMode midi;
static GamepadMode gamepad;

Mode* Modes[] = {
  &serial,
  &keys1,
  &keys2,
  &midi,
  &gamepad
};
