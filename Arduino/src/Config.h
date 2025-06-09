// Config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h> // Using v7
#include <LittleFS.h>

// Define maximums for arrays - adjust if you need more
#define MAX_BUTTON_ROWS 5
#define MAX_BUTTON_COLS 5
#define MAX_POTENTIOMETERS 4
#define MAX_ROTARY_ENCODERS 2

#define CONFIG_FILE "/config.json"

class ConfigManager {
public:
  ConfigManager();
  void begin(); // Loads config or creates default
  bool loadConfig();
  bool saveConfig();
  void createDefaultConfig();
  void printConfig(); // For debugging

  // General
  int getMaxLayers() const { return maxLayers; }
  int getLedBuiltinPin() const { return ledBuiltinPin; }

  // Buttons
  bool areButtonsEnabled() const { return buttons_enabled; }
  byte getNumButtonRows() const { return buttons_numRows; }
  byte getNumButtonCols() const { return buttons_numCols; }
  const byte* getButtonRowPins() const { return buttons_rowPins; }
  const byte* getButtonColPins() const { return buttons_colPins; }
  byte getButtonRowPin(int index) const { return (index < buttons_numRows) ? buttons_rowPins[index] : 255; }
  byte getButtonColPin(int index) const { return (index < buttons_numCols) ? buttons_colPins[index] : 255; }
  unsigned long getButtonDebounceDelay() const { return buttons_debounceDelay; }

  // Potentiometers
  bool arePotentiometersEnabled() const { return potentiometers_enabled; }
  byte getNumPots() const { return potentiometers_numPots; }
  const byte* getPotPins() const { return potentiometers_potPins; }
  byte getPotPin(int index) const { return (index < potentiometers_numPots) ? potentiometers_potPins[index] : 255; }
  float getPotAlpha() const { return potentiometers_alpha; }
  int getPotReportThreshold() const { return potentiometers_reportThreshold; }

  // Rotary Encoders
  bool areRotaryEncodersEnabled() const { return rotaryEncoders_enabled; }
  byte getNumEncoders() const { return rotaryEncoders_numEncoders; }
  const byte* getEncoderPinsA() const { return rotaryEncoders_encoderPinsA; }
  const byte* getEncoderPinsB() const { return rotaryEncoders_encoderPinsB; }
  byte getEncoderPinA(int index) const { return (index < rotaryEncoders_numEncoders) ? rotaryEncoders_encoderPinsA[index] : 255; }
  byte getEncoderPinB(int index) const { return (index < rotaryEncoders_numEncoders) ? rotaryEncoders_encoderPinsB[index] : 255; }
  
  bool updateConfigFromSerial(Stream &input); // Returns true if config updated and saved

private:
  // General
  int maxLayers;
  int ledBuiltinPin;

  // Buttons
  bool buttons_enabled;
  byte buttons_numRows;
  byte buttons_numCols;
  byte buttons_rowPins[MAX_BUTTON_ROWS];
  byte buttons_colPins[MAX_BUTTON_COLS];
  unsigned long buttons_debounceDelay;

  // Potentiometers
  bool potentiometers_enabled;
  byte potentiometers_numPots;
  byte potentiometers_potPins[MAX_POTENTIOMETERS];
  float potentiometers_alpha;
  int potentiometers_reportThreshold;

  // Rotary Encoders
  bool rotaryEncoders_enabled;
  byte rotaryEncoders_numEncoders;
  byte rotaryEncoders_encoderPinsA[MAX_ROTARY_ENCODERS];
  byte rotaryEncoders_encoderPinsB[MAX_ROTARY_ENCODERS];

  bool parseJsonToConfig(JsonDocument& doc);
};

extern ConfigManager configManager;

#endif