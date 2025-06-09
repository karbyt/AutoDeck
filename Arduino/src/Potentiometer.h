// Potentiometer.h
#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <Arduino.h>
#include "Config.h" // For MAX_POTENTIOMETERS

class PotManager {
public:
  void setup(); // Will use ConfigManager
  void update(int currentLayer); // <--- TAMBAHKAN parameter currentLayer
  
private:
  // Actual number of sliders used, from config
  byte activePots;
  float configuredAlpha;
  int configuredReportThreshold;
  
  // State arrays, sized by maximums
  // Kept static as there's one global PotManager instance
  static float smoothedValue[MAX_POTENTIOMETERS];
  static int lastReportedValue[MAX_POTENTIOMETERS];
};

extern PotManager potentiometer;

#endif