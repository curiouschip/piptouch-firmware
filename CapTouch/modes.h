#ifndef MODES_H
#define MODES_H

#include <Arduino.h>
#include <Keyboard.h>
#include "simple_midi.h"
#include "Joystick.h"
#include <stdint.h>
#include "console.h"
#include "settings.h"

class Mode {
public:
  void activate() {
    buttonsPrev = 0;
    sliderPrev = -1;
    activateHardware();
  }

  void deactivate() {
    deactivateHardware();
  }

  void update(uint8_t buttons, int slider) {
    buttonsCurr = buttons;
    sliderCurr = slider;
    process();
    buttonsPrev = buttonsCurr;
    sliderPrev = sliderCurr;
  }

protected:
  virtual void activateHardware() = 0;
  virtual void deactivateHardware() = 0;
  virtual void process() = 0;

  bool changed() {
    return (buttonsPrev != buttonsCurr) || (sliderPrev != sliderCurr);
  }

  byte buttonsPrev, buttonsCurr;
  int sliderPrev, sliderCurr;
};

class SerialMode : public Mode {
protected:
  void activateHardware() {}
  void deactivateHardware() {}
 
  void process() {
    if (!changed()) {
      return;
    }
    
    uint8_t b = buttonsCurr;
    CONSOLE_PORT.print("> ");
    for (int i = 0; i < 8; ++i) {
      CONSOLE_PORT.print((b & 0x01) ? 'X' : '_');
      b >>= 1;  
    }

    CONSOLE_PORT.print(" ");
    CONSOLE_PORT.print(sliderCurr);
    CONSOLE_PORT.println();
  }
};

class KeyboardMode : public Mode {
protected:
  KeyboardMode(char *keys) : keyMap(keys) {}

  void activateHardware() {
    Keyboard.begin();
  }

  void deactivateHardware() {
    Keyboard.releaseAll();
    Keyboard.end();
  }
  
  void process() {
    for (int i = 0; i < 8; ++i) {
      bool wasPressed = buttonsPrev & (1 << i);
      bool isPressed = buttonsCurr & (1 << i);
      if (!wasPressed && isPressed) {
        Keyboard.press(keyMap[i]);
      } else if (wasPressed && !isPressed) {
        Keyboard.release(keyMap[i]);
      }
    }
  }

private:
  char *keyMap;
};

static char numericKeyMap[] = {
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8'
};

class NumericKeyboardMode : public KeyboardMode {
public:
  NumericKeyboardMode() : KeyboardMode(numericKeyMap) {}
};

static char cursorKeyMap[] = {
  KEY_UP_ARROW,
  KEY_DOWN_ARROW,
  KEY_LEFT_ARROW,
  KEY_RIGHT_ARROW,
  ' ',
  KEY_RETURN,
  KEY_BACKSPACE,
  KEY_ESC
};

class CursorKeyboardMode : public KeyboardMode {
public:
  CursorKeyboardMode() : KeyboardMode(cursorKeyMap) {}  
};

static char midiNoteMap[] = {
  60,
  62,
  64,
  65,
  67,
  69,
  71,
  72
};

class MIDIMode : public Mode {
protected:
  void activateHardware() {}
  void deactivateHardware() {}

  void process() {
    for (int i = 0; i < 8; ++i) {
      bool wasPressed = buttonsPrev & (1 << i);
      bool isPressed = buttonsCurr & (1 << i);
      if (!wasPressed && isPressed) {
        midiEventPacket_t evt = { 0x09, 0x90 | settings_get_midi_channel(), midiNoteMap[i], 127 };
        MIDI.sendMIDI(evt);
      } else if (wasPressed && !isPressed) {
        midiEventPacket_t evt = { 0x08, 0x80 | settings_get_midi_channel(), midiNoteMap[i], 0 };
        MIDI.sendMIDI(evt);
      }
    }
    if (sliderCurr != sliderPrev) {
      uint8_t controller = settings_get_midi_controller();
      if (controller == 0) {
        uint16_t pitch = 0x2000;
        if (sliderCurr >= 0) {
          pitch = mapRange(0, 255, 0, 0x3FFF, sliderCurr);
        }
        midiEventPacket_t evt = { 0x0E, 0xE0 | settings_get_midi_channel(), pitch & 0x7F, (pitch >> 7) & 0x7F };
        MIDI.sendMIDI(evt);
      } else {
        uint16_t val = 0;
        if (sliderCurr >= 0) {
          val = mapRange(0, 255, 0, 127, sliderCurr);
        }
        midiEventPacket_t evt = { 0x0B, 0xB0 | settings_get_midi_channel(), controller, val };
        MIDI.sendMIDI(evt);
      }
    }
    MIDI.flush();
  }

private:
  long mapRange(long in1, long in2, long out1, long out2, long v) {
    long a = v - in1;
    long b = out2 - out1;
    long c = a * b;
    long d = in2 - in1;
    return out1 + c / d;
  }
};

class GamepadMode : public Mode {
public:
  GamepadMode() : stick(
    0x03, // hidReportId
    JOYSTICK_TYPE_GAMEPAD, // type
    4, // buttonCount
    0, // hatSwitchCount
    true, // includeXAxis
    true, // includeYAxis
    true, // includeZAxis
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false
  ) {
    stick.setXAxisRange(-128, 127);
    stick.setYAxisRange(-128, 127);
    stick.setZAxisRange(-128, 127);
    stick.begin(false);
  }
  
protected:
  void activateHardware() {}
  void deactivateHardware() {}

  void process() {
    uint8_t b = buttonsCurr;
    
    bool up = b & 0x01;
    bool down = b & 0x02;
    if (up && !down) {
      stick.setYAxis(-128);
    } else if (!up && down) {
      stick.setYAxis(127);
    } else {
      stick.setYAxis(0);
    }

    bool left = b & 0x04;
    bool right = b & 0x08;
    if (left && !right) {
      stick.setXAxis(-128);
    } else if (!left && right) {
      stick.setXAxis(127);
    } else {
      stick.setXAxis(0);
    }

    b >>= 4;
    for (int i = 0; i < 4; ++i) {
      stick.setButton(i, (b & 0x01) ? 1 : 0);
      b >>= 1;
    }

    int16_t slider = sliderCurr;
    if (slider < 0) {
      slider = 128;
    }
    stick.setZAxis(slider - 128);

    stick.sendState();
  }

private:
  Joystick_ stick;
};

#define MODE_COUNT 5
extern Mode* Modes[];

#endif
