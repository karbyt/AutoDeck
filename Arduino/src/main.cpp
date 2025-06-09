// YourProjectName.ino
#include "Config.h"
#include "Layer.h"
#include "Button.h"
#include "Potentiometer.h"
#include "RotaryEncoder.h"

String serialBuffer = ""; // Buffer for incoming serial data (especially for JSON config)
bool receivingConfig = false; // Flag to indicate if we are in the middle of receiving a JSON config


void handleSerialInput() {
  while (Serial.available() > 0) {
    char incomingChar = Serial.read();
    serialBuffer += incomingChar;

    // Simple check for JSON: starts with '{', ends with '}' and try to parse
    // This assumes the JSON is sent in one go or buffered correctly.
    // A more robust solution would handle multi-line JSON input.
    if (serialBuffer.startsWith("{") && !receivingConfig) {
        receivingConfig = true;
        Serial.println("Receiving JSON config...");
    }

    if (receivingConfig) {
        // Check if the buffer seems to contain a complete JSON object
        // This is a heuristic. A proper parser would count braces.
        if (incomingChar == '}' ) { // Check for the final brace of the JSON
            // Attempt to parse the potential JSON.
            // Count open and close braces to be more robust
            int openBraces = 0;
            int closeBraces = 0;
            for (int i=0; i < serialBuffer.length(); i++) {
                if (serialBuffer.charAt(i) == '{') openBraces++;
                if (serialBuffer.charAt(i) == '}') closeBraces++;
            }

            if (openBraces > 0 && openBraces == closeBraces) {
                Serial.println("Potential JSON config received. Attempting to parse:");
                Serial.print("ACK: ");
                Serial.println(serialBuffer);
                // Create a stream from the string to pass to configManager
                // This is a bit clunky. ArduinoJson can deserialize from String directly.
                // Let's modify updateConfigFromSerial to accept a String or const char*

                // Temporarily, let's make a copy for a C-string.
                // Or better, ArduinoJson V7 can deserialize from String.
                // We'll need to modify ConfigManager::updateConfigFromSerial to accept a String.
                // For now, let's assume a simpler direct call if Serial was the stream:
                // configManager.updateConfigFromSerial(Serial); // if we buffered perfectly.
                // Since we are buffering into `serialBuffer`, we need a way to feed this.
                // The easiest is if updateConfigFromSerial can take a String.
                // For now, let's simplify and assume the JSON is sent in one line
                // and the next part of the if/else handles it if it's not a layer command.
                // This part will be handled in the else if below
            }
        }
    }


    // Process complete lines (for L: commands or single-line JSON)
    if (incomingChar == '\n') {
      serialBuffer.trim(); // Remove newline and any whitespace

      if (serialBuffer.startsWith("L:")) {
        int layer = serialBuffer.substring(2).toInt();
        if (layer > 0) {
          layerManager.setCurrentLayer(layer);
        } else {
          Serial.println("Invalid layer number.");
        }
        receivingConfig = false; // Not a config
      } else if (serialBuffer.startsWith("{") && serialBuffer.endsWith("}")) {
        Serial.println("JSON config string detected. Processing...");
        // Create a custom stream wrapper for the String
        class StringStream : public Stream {
          private:
            String _str;
            int _pos;
          public:
            StringStream(const String& s) : _str(s), _pos(0) {}
            virtual int available() { return _str.length() - _pos; }
            virtual int read() { return _pos < _str.length() ? _str.charAt(_pos++) : -1; }
            virtual int peek() { return _pos < _str.length() ? _str.charAt(_pos) : -1; }
            virtual void flush() {} // No-op
            virtual size_t write(uint8_t c) { return 0; } // Not used for reading
        };
        StringStream jsonStream(serialBuffer);
        configManager.updateConfigFromSerial(jsonStream); // This will restart if successful
        // If it failed, it won't restart, so we might want to clear buffer or give feedback
      } else if (!serialBuffer.isEmpty()) {
        Serial.print("Unknown command: ");
        Serial.println(serialBuffer);
      }
      serialBuffer = ""; // Clear buffer for next command
      receivingConfig = false; // Reset config receiving flag
    }
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial connection
  Serial.println("\nBooting up...");

  configManager.begin(); // Loads or creates default config from LittleFS
  layerManager.begin(configManager.getMaxLayers()); // Initialize layer manager with max layers from config

  // Setup peripherals - they will use the global configManager
  if(configManager.areButtonsEnabled()) button.setup();
  if(configManager.arePotentiometersEnabled()) potentiometer.setup();
  if(configManager.areRotaryEncodersEnabled()) rotary.setup();

  Serial.println("Setup complete. Ready.");
  Serial.println("Commands: L:<layer_num>");
  Serial.println("          Send JSON config string (e.g., {...}) to update config.");
}

void loop() {
  handleSerialInput();

  int currentLayer = layerManager.getCurrentLayer(); // Ambil layer saat ini

  if(configManager.areButtonsEnabled()) button.update(currentLayer);
  // PERBAIKAN: Kirim currentLayer ke update()
  if(configManager.arePotentiometersEnabled()) potentiometer.update(currentLayer); 
  // PERBAIKAN: Kirim currentLayer ke update()
  if(configManager.areRotaryEncodersEnabled()) rotary.update(currentLayer); 
  
  // delay(1); // Opsional
}

