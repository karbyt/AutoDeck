// RotaryEncoder.h
#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>
#include "Config.h" // For MAX_ROTARY_ENCODERS

class RotaryManager {
public:
  void setup(); // Will use ConfigManager
  void update(int currentLayer); // <--- TAMBAHKAN parameter currentLayer

private:
  byte activeEncoders; // Actual number of encoders used, from config
  // State array, sized by maximums
  // Kept static as there's one global RotaryManager instance
  static int lastStateA[MAX_ROTARY_ENCODERS];
};

extern RotaryManager rotary;

#endif