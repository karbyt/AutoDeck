// Button.h
#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>
#include "Config.h" // For MAX_BUTTON_ROWS, MAX_BUTTON_COLS

class Button {
public:
  void setup(); // Will use ConfigManager
  void update(int currentLayer); // Pass current layer for ID calculation
  
private:
  // These are no longer static const, they'll be set from ConfigManager
  // Static members for state arrays, sized by maximums
  // We keep them static because there's one global Button instance managing the physical matrix
  static bool previousKeyStates[MAX_BUTTON_ROWS][MAX_BUTTON_COLS];
  static unsigned long lastDebounceTime[MAX_BUTTON_ROWS][MAX_BUTTON_COLS];
  static bool debouncingKeyState[MAX_BUTTON_ROWS][MAX_BUTTON_COLS];

  // Actual number of rows/cols used, from config
  byte activeRows;
  byte activeCols;
  unsigned long configuredDebounceDelay;
  // Pin arrays will be accessed via ConfigManager
};

extern Button button;

#endif