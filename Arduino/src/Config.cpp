// Config.cpp
#include "Config.h"

ConfigManager::ConfigManager() {
  // Initialize with some very basic defaults in case createDefaultConfig isn't immediately called
  maxLayers = 1;
  ledBuiltinPin = 2; // Default LED_BUILTIN for ESP32

  buttons_enabled = false;
  buttons_numRows = 0;
  buttons_numCols = 0;
  buttons_debounceDelay = 50;

  potentiometers_enabled = false;
  potentiometers_numPots = 0;
  potentiometers_alpha = 0.1;
  potentiometers_reportThreshold = 10;

  rotaryEncoders_enabled = false;
  rotaryEncoders_numEncoders = 0;
}

void ConfigManager::begin() {
  if (!LittleFS.begin(true)) { // true = format if mount failed
    Serial.println("LittleFS Mount Failed. Using default config.");
    createDefaultConfig(); // Can't save, but at least we have a config
    return;
  }
  Serial.println("LittleFS Mounted.");
  if (!loadConfig()) {
    Serial.println("Failed to load config or config not found. Creating default config.");
    createDefaultConfig();
    saveConfig();
  } else {
    Serial.println("Config loaded successfully.");
  }
  printConfig(); // For debugging
}

void ConfigManager::createDefaultConfig() {
  Serial.println("Creating default configuration...");
  maxLayers = 3;
  ledBuiltinPin = 2;

  // Buttons
  buttons_enabled = true;
  buttons_numRows = 3;
  buttons_numCols = 4;
  const byte default_rowPins[3] = {13, 12, 14};
  const byte default_colPins[4] = {33, 32, 15, 4};
  memcpy(buttons_rowPins, default_rowPins, sizeof(default_rowPins));
  memcpy(buttons_colPins, default_colPins, sizeof(default_colPins));
  buttons_debounceDelay = 50;

  // Potentiometers
  potentiometers_enabled = true;
  potentiometers_numPots = 2;
  const byte default_potPins[2] = {34, 35};
  memcpy(potentiometers_potPins, default_potPins, sizeof(default_potPins));
  potentiometers_alpha = 0.05;
  potentiometers_reportThreshold = 20;

  // Rotary Encoders
  rotaryEncoders_enabled = true;
  rotaryEncoders_numEncoders = 1;
  const byte default_encoderPinsA[1] = {17};
  const byte default_encoderPinsB[1] = {5};
  memcpy(rotaryEncoders_encoderPinsA, default_encoderPinsA, sizeof(default_encoderPinsA));
  memcpy(rotaryEncoders_encoderPinsB, default_encoderPinsB, sizeof(default_encoderPinsB));
}

bool ConfigManager::parseJsonToConfig(JsonDocument& doc) {
  // General
  maxLayers = doc["general"]["maxLayers"] | 1; // Default to 1 if not found
  ledBuiltinPin = doc["general"]["ledBuiltinPin"] | 2;

  // Buttons
  JsonObject buttonsSection = doc["buttons"];
  buttons_enabled = buttonsSection["enabled"] | false;
  if (buttons_enabled) {
    buttons_numRows = buttonsSection["numRows"] | 0;
    buttons_numCols = buttonsSection["numCols"] | 0;
    JsonArrayConst rowPinsArray = buttonsSection["rowPins"];
    JsonArrayConst colPinsArray = buttonsSection["colPins"];
    if (buttons_numRows > MAX_BUTTON_ROWS) buttons_numRows = MAX_BUTTON_ROWS;
    if (buttons_numCols > MAX_BUTTON_COLS) buttons_numCols = MAX_BUTTON_COLS;

    for (byte i = 0; i < buttons_numRows; ++i) {
      buttons_rowPins[i] = rowPinsArray[i] | 0; // Default to pin 0 if error
    }
    for (byte i = 0; i < buttons_numCols; ++i) {
      buttons_colPins[i] = colPinsArray[i] | 0;
    }
    buttons_debounceDelay = buttonsSection["debounceDelay"] | 50;
  } else {
    buttons_numRows = 0;
    buttons_numCols = 0;
  }


  // Potentiometers
  JsonObject potsSection = doc["potentiometers"];
  potentiometers_enabled = potsSection["enabled"] | false;
  if (potentiometers_enabled) {
    potentiometers_numPots = potsSection["numPots"] | 0;
    JsonArrayConst potPinsArray = potsSection["potPins"];
    if (potentiometers_numPots > MAX_POTENTIOMETERS) potentiometers_numPots = MAX_POTENTIOMETERS;

    for (byte i = 0; i < potentiometers_numPots; ++i) {
      potentiometers_potPins[i] = potPinsArray[i] | 0;
    }
    potentiometers_alpha = potsSection["alpha"] | 0.1f;
    potentiometers_reportThreshold = potsSection["reportThreshold"] | 10;
  } else {
    potentiometers_numPots = 0;
  }

  // Rotary Encoders
  JsonObject rotarySection = doc["rotaryEncoders"];
  rotaryEncoders_enabled = rotarySection["enabled"] | false;
  if (rotaryEncoders_enabled) {
    rotaryEncoders_numEncoders = rotarySection["numEncoders"] | 0;
    JsonArrayConst encoderPinsAArray = rotarySection["encoderPinsA"];
    JsonArrayConst encoderPinsBArray = rotarySection["encoderPinsB"];
    if (rotaryEncoders_numEncoders > MAX_ROTARY_ENCODERS) rotaryEncoders_numEncoders = MAX_ROTARY_ENCODERS;
    
    for (byte i = 0; i < rotaryEncoders_numEncoders; ++i) {
      rotaryEncoders_encoderPinsA[i] = encoderPinsAArray[i] | 0;
      rotaryEncoders_encoderPinsB[i] = encoderPinsBArray[i] | 0;
    }
  } else {
    rotaryEncoders_numEncoders = 0;
  }
  return true; // Assuming parsing itself doesn't fail catastrophically with defaults
}


bool ConfigManager::loadConfig() {
  File configFile = LittleFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return false;
  }

  JsonDocument doc; // For ArduinoJson v7, dynamic allocation by default
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
  }

  return parseJsonToConfig(doc);
}

bool ConfigManager::saveConfig() {
    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println("Failed to open config file for writing");
      return false;
    }
  
    JsonDocument doc; // Kapasitas disesuaikan otomatis oleh ArduinoJson v7
  
    // General
    doc["general"]["maxLayers"] = maxLayers;
    doc["general"]["ledBuiltinPin"] = ledBuiltinPin;
  
    // Buttons
    doc["buttons"]["enabled"] = buttons_enabled;
    doc["buttons"]["numRows"] = buttons_numRows;
    doc["buttons"]["numCols"] = buttons_numCols;
    // PERBAIKAN: Gunakan .to<JsonArray>()
    JsonArray rowPins = doc["buttons"]["rowPins"].to<JsonArray>(); 
    for (int i = 0; i < buttons_numRows; i++) rowPins.add(buttons_rowPins[i]);
    // PERBAIKAN: Gunakan .to<JsonArray>()
    JsonArray colPins = doc["buttons"]["colPins"].to<JsonArray>(); 
    for (int i = 0; i < buttons_numCols; i++) colPins.add(buttons_colPins[i]);
    doc["buttons"]["debounceDelay"] = buttons_debounceDelay;
  
    // Potentiometers
    doc["potentiometers"]["enabled"] = potentiometers_enabled;
    doc["potentiometers"]["numPots"] = potentiometers_numPots;
    // PERBAIKAN: Gunakan .to<JsonArray>()
    JsonArray potPins = doc["potentiometers"]["potPins"].to<JsonArray>(); 
    for (int i = 0; i < potentiometers_numPots; i++) potPins.add(potentiometers_potPins[i]);
    doc["potentiometers"]["alpha"] = potentiometers_alpha;
    doc["potentiometers"]["reportThreshold"] = potentiometers_reportThreshold;
  
    // Rotary Encoders
    doc["rotaryEncoders"]["enabled"] = rotaryEncoders_enabled;
    doc["rotaryEncoders"]["numEncoders"] = rotaryEncoders_numEncoders;
    // PERBAIKAN: Gunakan .to<JsonArray>()
    JsonArray encAPins = doc["rotaryEncoders"]["encoderPinsA"].to<JsonArray>(); 
    for (int i = 0; i < rotaryEncoders_numEncoders; i++) encAPins.add(rotaryEncoders_encoderPinsA[i]);
    // PERBAIKAN: Gunakan .to<JsonArray>()
    JsonArray encBPins = doc["rotaryEncoders"]["encoderPinsB"].to<JsonArray>(); 
    for (int i = 0; i < rotaryEncoders_numEncoders; i++) encBPins.add(rotaryEncoders_encoderPinsB[i]);
  
    if (serializeJson(doc, configFile) == 0) {
      Serial.println("Failed to write to config file");
      configFile.close();
      return false;
    }
    configFile.close();
    Serial.println("Config saved.");
    return true;
  }

bool ConfigManager::updateConfigFromSerial(Stream &input) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, input);

  if (error) {
    Serial.print("deserializeJson() failed during update: ");
    Serial.println(error.c_str());
    return false;
  }

  Serial.println("New config JSON received and parsed. Updating values.");
  // Parse new values into member variables
  if (!parseJsonToConfig(doc)) {
      Serial.println("Error applying parsed JSON to config structure.");
      return false;
  }
  
  Serial.println("Saving new config...");
  if (saveConfig()) {
    Serial.println("New config saved. Restarting device.");
    printConfig(); // Print the new config before restart
    delay(100); // Give serial time to send
    ESP.restart();
    return true; // Though it won't reach here due to restart
  } else {
    Serial.println("Failed to save new config.");
    return false;
  }
}

void ConfigManager::printConfig() {
  Serial.println("--- Current Configuration ---");
  Serial.println("ACK: ");
  Serial.print("Max Layers: "); Serial.println(maxLayers);
  Serial.print("LED Pin: "); Serial.println(ledBuiltinPin);

  Serial.println("[Buttons]");
  Serial.print("  Enabled: "); Serial.println(buttons_enabled ? "Yes" : "No");
  if (buttons_enabled) {
    Serial.print("  Rows: "); Serial.println(buttons_numRows);
    Serial.print("  Cols: "); Serial.println(buttons_numCols);
    Serial.print("  Row Pins: ");
    for(int i=0; i<buttons_numRows; i++) { Serial.print(buttons_rowPins[i]); Serial.print(" ");}
    Serial.println();
    Serial.print("  Col Pins: ");
    for(int i=0; i<buttons_numCols; i++) { Serial.print(buttons_colPins[i]); Serial.print(" ");}
    Serial.println();
    Serial.print("  Debounce: "); Serial.println(buttons_debounceDelay);
  }

  Serial.println("[Potentiometers]");
  Serial.print("  Enabled: "); Serial.println(potentiometers_enabled ? "Yes" : "No");
   if (potentiometers_enabled) {
    Serial.print("  Count: "); Serial.println(potentiometers_numPots);
    Serial.print("  Pins: ");
    for(int i=0; i<potentiometers_numPots; i++) { Serial.print(potentiometers_potPins[i]); Serial.print(" ");}
    Serial.println();
    Serial.print("  Alpha: "); Serial.println(potentiometers_alpha);
    Serial.print("  Threshold: "); Serial.println(potentiometers_reportThreshold);
  }

  Serial.println("[Rotary Encoders]");
  Serial.print("  Enabled: "); Serial.println(rotaryEncoders_enabled ? "Yes" : "No");
  if (rotaryEncoders_enabled) {
    Serial.print("  Count: "); Serial.println(rotaryEncoders_numEncoders);
    Serial.print("  Pins A: ");
    for(int i=0; i<rotaryEncoders_numEncoders; i++) { Serial.print(rotaryEncoders_encoderPinsA[i]); Serial.print(" ");}
    Serial.println();
    Serial.print("  Pins B: ");
    for(int i=0; i<rotaryEncoders_numEncoders; i++) { Serial.print(rotaryEncoders_encoderPinsB[i]); Serial.print(" ");}
    Serial.println();
  }
  Serial.println("---------------------------");
}

ConfigManager configManager; // Global instance