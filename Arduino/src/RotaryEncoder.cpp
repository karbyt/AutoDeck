#include "RotaryEncoder.h"

// Jumlah rotary encoder
const uint8_t NUM_ENCODERS = 1;

// Pin A dan B untuk masing-masing encoder
const uint8_t encoderPinsA[NUM_ENCODERS] = {17};
const uint8_t encoderPinsB[NUM_ENCODERS] = {5};

// State sebelumnya dari pin A
static int lastStateA[NUM_ENCODERS];

// Class implementation
void RotaryManager::setup() {
  for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
    pinMode(encoderPinsA[i], INPUT_PULLUP);
    pinMode(encoderPinsB[i], INPUT_PULLUP);
    lastStateA[i] = digitalRead(encoderPinsA[i]);
  }
}

void RotaryManager::update() {
  for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
    int currentStateA = digitalRead(encoderPinsA[i]);

    if (currentStateA != lastStateA[i]) {
      if (currentStateA == LOW) {
        int stateB = digitalRead(encoderPinsB[i]);
        Serial.print("R");
        Serial.print(i + 1); // ID encoder dimulai dari 1
        Serial.print(":");
        Serial.println(stateB != currentStateA ? "CW" : "CCW");
      }
    }

    lastStateA[i] = currentStateA;
  }
}

// Global instance
RotaryManager rotary;
