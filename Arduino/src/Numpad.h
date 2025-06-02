#ifndef NUMPAD_H
#define NUMPAD_H

#include <Arduino.h>

class Numpad {
public:
  void setup();
  void update();
  
private:
  static const byte rowPins[3];
  static const byte colPins[4];
  static const byte numRows = 3;
  static const byte numCols = 4;
  static const unsigned long debounceDelay = 50;

  static bool previousKeyStates[numRows][numCols];
  static unsigned long lastDebounceTime[numRows][numCols];
  static bool debouncingKeyState[numRows][numCols];
};

extern Numpad numpad;

#endif
