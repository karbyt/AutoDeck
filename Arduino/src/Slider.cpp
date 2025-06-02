#include "Slider.h"

// Konfigurasi
const uint8_t NUM_SLIDERS = 2;
const uint8_t sliderPins[NUM_SLIDERS] = {34, 35};

// Parameter filter
const float alpha = 0.05; // Semakin kecil = semakin halus
const int reportThreshold = 20; // Deadband untuk mencegah spam

// Nilai terakhir (float untuk presisi EMA), dan nilai yang sudah dilaporkan
static float smoothedValue[NUM_SLIDERS];
static int lastReportedValue[NUM_SLIDERS];

void SliderManager::setup() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(sliderPins[i], INPUT);
    int initial = analogRead(sliderPins[i]);
    smoothedValue[i] = initial;
    lastReportedValue[i] = initial;
  }
}

void SliderManager::update() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    int raw = analogRead(sliderPins[i]);
    
    // EMA filter
    smoothedValue[i] = (alpha * raw) + ((1.0 - alpha) * smoothedValue[i]);

    int rounded = round(smoothedValue[i]);

    // Hanya kirim jika beda cukup besar dari terakhir yang dilaporkan
    if (abs(rounded - lastReportedValue[i]) >= reportThreshold) {
      Serial.print("S");
      Serial.print(i + 1);
      Serial.print(":");
      Serial.println(rounded);
      lastReportedValue[i] = rounded;
    }
  }
}
SliderManager slider;