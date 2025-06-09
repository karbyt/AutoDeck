// RotaryEncoder.cpp
#include "RotaryEncoder.h"
#include "Config.h" // To get pin configurations and counts

// Static member definition
int RotaryManager::lastStateA[MAX_ROTARY_ENCODERS];

void RotaryManager::setup() {
  if (!configManager.areRotaryEncodersEnabled()) {
    Serial.println("Rotary Encoders module disabled in config.");
    activeEncoders = 0;
    return;
  }
  activeEncoders = configManager.getNumEncoders();
  if (activeEncoders == 0) {
    Serial.println("Number of rotary encoders is zero. Disabling rotary encoders.");
    return;
  }
  Serial.print("Rotary Encoder setup: Count="); Serial.println(activeEncoders);

  for (uint8_t i = 0; i < activeEncoders; i++) {
    pinMode(configManager.getEncoderPinA(i), INPUT_PULLUP);
    pinMode(configManager.getEncoderPinB(i), INPUT_PULLUP);
    lastStateA[i] = digitalRead(configManager.getEncoderPinA(i));
  }
  Serial.println("Rotary Encoders initialized.");
}

// PERBAIKAN: Ubah signature dan tambahkan logika layer
void RotaryManager::update(int currentLayer) {
  if (activeEncoders == 0 || !configManager.areRotaryEncodersEnabled()) {
    return; // Not configured or disabled
  }

  for (uint8_t i = 0; i < activeEncoders; i++) {
    int currentStateA = digitalRead(configManager.getEncoderPinA(i));

    if (currentStateA != lastStateA[i]) {
      if (currentStateA == LOW) { 
        int stateB = digitalRead(configManager.getEncoderPinB(i));

        // PERBAIKAN: Kalkulasi ID Encoder dengan Layer
        int baseEncoderID = i + 1; // ID dasar (1-based)
        int totalEncodersPerLayer = activeEncoders; // Sama dengan configManager.getNumEncoders()
        int finalEncoderID = baseEncoderID + ((currentLayer - 1) * totalEncodersPerLayer);

        Serial.print("R");
        Serial.print(finalEncoderID); // Gunakan ID yang sudah dilayer
        Serial.print(":");
        Serial.println(stateB == HIGH ? "CW" : "CCW"); 
      }
    }
    lastStateA[i] = currentStateA;
  }
}

// Global instance
RotaryManager rotary;