#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "build_info.h"

// === CONFIG ===
const char* WIFI_SSID = "Ender_2G";
const char* WIFI_PASS = "134679tdg";
const char* VERSION_JSON_URL = "http://enderekici.github.io/esp32c3/version.json";
const unsigned long CHECK_INTERVAL = 5 * 60 * 1000;
#define LED_PIN 8

// === GLOBALS ===
unsigned long lastCheck = 0;
String currentVersion = BUILD_VERSION;
bool otaInProgress = false;

// === OTA FUNCTION ===
bool doOTA(const String& firmware_url) {
  if (otaInProgress) return false;
  otaInProgress = true;

  HTTPClient http;
  http.begin(firmware_url);
  int code = http.GET();

  if (code != 200) { http.end(); otaInProgress = false; return false; }
  int len = http.getSize();
  WiFiClient* stream = http.getStreamPtr();
  if (!Update.begin(len)) { http.end(); otaInProgress = false; return false; }

  Update.writeStream(*stream);
  if (Update.end() && Update.isFinished()) {
    delay(1000); ESP.restart(); return true;
  } else { http.end(); otaInProgress = false; return false; }
}

// === FETCH LATEST VERSION INFO ===
bool fetchLatestVersion(String& latest_version, String& firmware_url) {
  HTTPClient http;
  http.begin(VERSION_JSON_URL);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) return false;

  latest_version = doc["version"].as<String>();
  firmware_url = doc["url"].as<String>();
  return true;
}

// === CHECK OTA HELPER ===
void checkForOTA() {
  String latest_version, firmware_url;
  if (!fetchLatestVersion(latest_version, firmware_url)) return;

  if (latest_version != currentVersion && !otaInProgress) {
    if (doOTA(firmware_url)) currentVersion = latest_version;
  }
}

// === SETUP / LOOP ===
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  lastCheck = millis();
}

void loop() {
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 500) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlink = millis();
  }

  if (millis() - lastCheck > CHECK_INTERVAL) {
    lastCheck = millis();
    checkForOTA();
  }
}
