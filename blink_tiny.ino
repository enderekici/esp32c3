// blink_tiny.ino
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "build_info.h"

#define LED_PIN 8           // Built-in LED
const char* WIFI_SSID = "Ender_2G";
const char* WIFI_PASS = "134679tdg";
const char* VERSION_JSON_URL = "https://enderekici.github.io/esp32c3/version.json";
const unsigned long CHECK_INTERVAL = 5 * 60 * 1000; // 5 minutes

unsigned long lastCheck = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  lastCheck = millis();
}

void loop() {
  // Blink LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 500) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }

  // Periodic OTA check
  if (millis() - lastCheck > CHECK_INTERVAL) {
    lastCheck = millis();
    checkForOTA();
  }
}

// === OTA HELPER ===
bool doOTA(const String& firmware_url) {
  Serial.println("Starting OTA update...");

  HTTPClient http;
  http.begin(firmware_url);
  int code = http.GET();

  if (code != 200) {
    Serial.printf("Failed to fetch firmware. HTTP code: %d\n", code);
    http.end();
    return false;
  }

  int len = http.getSize();
  WiFiClient *stream = http.getStreamPtr();

  if (len <= 0) {
    Serial.println("No firmware found at URL");
    http.end();
    return false;
  }

  if (!Update.begin(len)) {
    Serial.println("Not enough space for OTA");
    http.end();
    return false;
  }

  Update.writeStream(*stream);
  if (Update.end() && Update.isFinished()) {
    Serial.println("OTA update complete! Rebooting...");
    delay(1000);
    ESP.restart();
    return true;
  } else {
    Serial.println("OTA update failed");
    http.end();
    return false;
  }
}

void checkForOTA() {
  HTTPClient http;
  http.begin(VERSION_JSON_URL);
  int code = http.GET();

  if (code != 200) {
    Serial.printf("Failed to fetch version.json. HTTP code: %d\n", code);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println("Failed to parse version.json");
    return;
  }

  String latest_version = doc["version"].as<String>();
  String firmware_url = doc["url"].as<String>();

  if (latest_version != BUILD_VERSION) {
    Serial.printf("New firmware: %s (current: %s)\n", latest_version.c_str(), BUILD_VERSION);
    doOTA(firmware_url);
  } else {
    Serial.println("Already up-to-date.");
  }
}
