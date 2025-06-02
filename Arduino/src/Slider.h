#ifndef SLIDER_H
#define SLIDER_H

#include <Arduino.h>

class SliderManager {
public:
  void setup();
  void update();
};

extern SliderManager slider;

#endif
