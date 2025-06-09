// Potentiometer.cpp
#include "Potentiometer.h"
#include "Config.h" // To get pin configurations and parameters

// Static member definitions
float PotManager::smoothedValue[MAX_POTENTIOMETERS];
int PotManager::lastReportedValue[MAX_POTENTIOMETERS];

void PotManager::setup() {
  if (!configManager.arePotentiometersEnabled()) {
    Serial.println("Potentiometers module disabled in config.");
    activePots = 0;
    return;
  }

  activePots = configManager.getNumPots();
  configuredAlpha = configManager.getPotAlpha();
  configuredReportThreshold = configManager.getPotReportThreshold();

  if (activePots == 0) {
    Serial.println("Number of potentiometers is zero. Disabling potentiometers.");
    return;
  }
  Serial.print("Potentiometer setup: Count="); Serial.println(activePots);

  for (uint8_t i = 0; i < activePots; i++) {
    pinMode(configManager.getPotPin(i), INPUT);
    int initial = analogRead(configManager.getPotPin(i));
    smoothedValue[i] = initial;
    lastReportedValue[i] = initial;
  }
  Serial.println("Potentiometers initialized.");
}

// PERBAIKAN: Ubah signature dan tambahkan logika layer
void PotManager::update(int currentLayer) { 
  if (activePots == 0 || !configManager.arePotentiometersEnabled()) {
    return; // Not configured or disabled
  }

  for (uint8_t i = 0; i < activePots; i++) {
    int raw = analogRead(configManager.getPotPin(i));
    
    smoothedValue[i] = (configuredAlpha * raw) + ((1.0 - configuredAlpha) * smoothedValue[i]);
    int rounded = round(smoothedValue[i]);

    if (abs(rounded - lastReportedValue[i]) >= configuredReportThreshold) {
      // PERBAIKAN: Kalkulasi ID Potensiometer dengan Layer
      int basePotID = i + 1; // ID dasar (1-based)
      int totalPotsPerLayer = activePots; // Sama dengan configManager.getNumPots()
      int finalPotID = basePotID + ((currentLayer - 1) * totalPotsPerLayer);

      Serial.print("P"); // Tetap P
      Serial.print(finalPotID); // Gunakan ID yang sudah dilayer
      Serial.print(":");
      Serial.println(rounded);
      lastReportedValue[i] = rounded;
    }
  }
}

PotManager potentiometer; // Global instance