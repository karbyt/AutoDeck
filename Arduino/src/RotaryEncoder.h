#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

class RotaryManager {
public:
  void setup();
  void update();
};

extern RotaryManager rotary;

#endif
