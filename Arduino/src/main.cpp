#include "Numpad.h"
#include "RotaryEncoder.h"
#include "Slider.h"

void setup() {
  Serial.begin(115200);
  numpad.setup();
  rotary.setup();
  slider.setup();
}

void loop() {
  numpad.update();
  rotary.update();
  slider.update();
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');  // membaca sampai newline
    Serial.print("ACK: ");                       // konfirmasi balik
    Serial.println(msg);                         // balasan dengan echo
  }
}
