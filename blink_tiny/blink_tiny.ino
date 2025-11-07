#include <Arduino.h>
#include "build_info.h"

const int LED_PIN = 8;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Blink Tiny sketch started!");
  Serial.printf("Version: %s, Commit: %s\n", BUILD_VERSION, BUILD_COMMIT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
