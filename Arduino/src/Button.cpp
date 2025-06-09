// Button.cpp
#include "Button.h"
#include "Config.h"   // To get pin configurations and counts
#include "Layer.h"    // Not strictly needed here if layer is passed to update()

// Define static members
bool Button::previousKeyStates[MAX_BUTTON_ROWS][MAX_BUTTON_COLS];
unsigned long Button::lastDebounceTime[MAX_BUTTON_ROWS][MAX_BUTTON_COLS];
bool Button::debouncingKeyState[MAX_BUTTON_ROWS][MAX_BUTTON_COLS];

// LED_BUILTIN is now managed by ConfigManager, but for simplicity we'll keep this define if not set elsewhere
#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // Default for many ESP32 dev boards
#endif

void Button::setup() {
  if (!configManager.areButtonsEnabled()) {
    Serial.println("Buttons module disabled in config.");
    activeRows = 0; // Mark as inactive
    activeCols = 0;
    return;
  }

  activeRows = configManager.getNumButtonRows();
  activeCols = configManager.getNumButtonCols();
  configuredDebounceDelay = configManager.getButtonDebounceDelay();

  if (activeRows == 0 || activeCols == 0) {
    Serial.println("Button rows or cols are zero in config. Disabling buttons.");
    activeRows = 0; // Ensure fully disabled
    return;
  }
  
  Serial.print("Button setup: Rows="); Serial.print(activeRows);
  Serial.print(", Cols="); Serial.println(activeCols);

  pinMode(configManager.getLedBuiltinPin(), OUTPUT);
  digitalWrite(configManager.getLedBuiltinPin(), LOW);

  for (byte r = 0; r < activeRows; r++) {
    pinMode(configManager.getButtonRowPin(r), OUTPUT);
    digitalWrite(configManager.getButtonRowPin(r), HIGH);
  }

  for (byte c = 0; c < activeCols; c++) {
    pinMode(configManager.getButtonColPin(c), INPUT_PULLUP);
  }

  for (byte r = 0; r < MAX_BUTTON_ROWS; r++) { // Initialize full static arrays
    for (byte c = 0; c < MAX_BUTTON_COLS; c++) {
      previousKeyStates[r][c] = false;
      lastDebounceTime[r][c] = 0;
      debouncingKeyState[r][c] = false;
    }
  }
  Serial.println("Buttons initialized.");
}

void Button::update(int currentLayer) {
  if (activeRows == 0 || activeCols == 0 || !configManager.areButtonsEnabled()) {
    return; // Not configured or disabled
  }

  bool anyKeyPressedThisScan = false;
  const byte* rowPins = configManager.getButtonRowPins();
  const byte* colPins = configManager.getButtonColPins();

  for (byte r = 0; r < activeRows; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(10); // Increased slightly for stability

    for (byte c = 0; c < activeCols; c++) {
      bool currentPhysicalState = (digitalRead(colPins[c]) == LOW);

      if (currentPhysicalState != debouncingKeyState[r][c]) {
        lastDebounceTime[r][c] = millis();
        debouncingKeyState[r][c] = currentPhysicalState;
      }

      if ((millis() - lastDebounceTime[r][c]) > configuredDebounceDelay) {
        if (debouncingKeyState[r][c] != previousKeyStates[r][c]) {
          previousKeyStates[r][c] = debouncingKeyState[r][c];

          // Layered Key ID calculation
          int baseKeyID = (r * activeCols) + c + 1;
          int totalKeysPerLayer = activeRows * activeCols;
          int finalKeyID = baseKeyID + ((currentLayer - 1) * totalKeysPerLayer);

          Serial.print("B"); // Changed from K to B
          Serial.print(finalKeyID);
          Serial.print(":");
          Serial.println(previousKeyStates[r][c] ? "P" : "R"); // P for Pressed, R for Released
        }
      }

      if (previousKeyStates[r][c]) {
        anyKeyPressedThisScan = true;
      }
    }
    digitalWrite(rowPins[r], HIGH);
  }
  digitalWrite(configManager.getLedBuiltinPin(), anyKeyPressedThisScan ? HIGH : LOW);
  // Consider removing or reducing this delay if it impacts responsiveness of other components
  // delay(5); // This was in the original code, might be too long for a main loop
}

// Global instance
Button button;