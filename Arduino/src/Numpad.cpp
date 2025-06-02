#include "Numpad.h"

// Define static members
const byte Numpad::rowPins[3] = {13, 12, 14};
const byte Numpad::colPins[4] = {33, 32, 15, 4};
bool Numpad::previousKeyStates[numRows][numCols];
unsigned long Numpad::lastDebounceTime[numRows][numCols];
bool Numpad::debouncingKeyState[numRows][numCols];

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

void Numpad::setup() {
  

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  for (byte r = 0; r < numRows; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }

  for (byte c = 0; c < numCols; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

  for (byte r = 0; r < numRows; r++) {
    for (byte c = 0; c < numCols; c++) {
      previousKeyStates[r][c] = false;
      lastDebounceTime[r][c] = 0;
      debouncingKeyState[r][c] = false;
    }
  }
}

void Numpad::update() {
  bool anyKeyPressedThisScan = false;

  for (byte r = 0; r < numRows; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(5);

    for (byte c = 0; c < numCols; c++) {
      bool currentPhysicalState = (digitalRead(colPins[c]) == LOW);

      if (currentPhysicalState != debouncingKeyState[r][c]) {
        lastDebounceTime[r][c] = millis();
        debouncingKeyState[r][c] = currentPhysicalState;
      }

      if ((millis() - lastDebounceTime[r][c]) > debounceDelay) {
        if (debouncingKeyState[r][c] != previousKeyStates[r][c]) {
          previousKeyStates[r][c] = debouncingKeyState[r][c];

          int keyID = (r * numCols) + c + 1;

          Serial.print("K");
          Serial.print(keyID);
          Serial.print(":");
          Serial.println(previousKeyStates[r][c] ? "P" : "R");
        }
      }

      if (previousKeyStates[r][c]) {
        anyKeyPressedThisScan = true;
      }
    }

    digitalWrite(rowPins[r], HIGH);
  }

  digitalWrite(LED_BUILTIN, anyKeyPressedThisScan ? HIGH : LOW);

  delay(5);
}

// Global instance
Numpad numpad;
