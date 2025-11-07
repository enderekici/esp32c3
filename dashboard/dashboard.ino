#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "build_info.h"

// CONFIG
const char* WIFI_SSID = "Ender_2G";
const char* WIFI_PASS = "134679tdg";
const char* VERSION_JSON_URL = "http://enderekici.github.io/esp32c3/version.json";
const unsigned long CHECK_INTERVAL = 5 * 60 * 1000; // 5 mins

// GLOBALS
WebServer server(80);
unsigned long lastCheck = 0;
bool otaInProgress = false;

// OTA Function
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

// Fetch Latest Version
bool fetchLatestVersion(String& latest_version, String& firmware_url) {
  HTTPClient http;
  http.begin(VERSION_JSON_URL);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload)) return false;

  latest_version = doc["version"].as<String>();
  firmware_url = doc["url"].as<String>();
  return true;
}

// OTA Check
void checkForOTA() {
  String latest_version, firmware_url;
  if (!fetchLatestVersion(latest_version, firmware_url)) return;

  if (latest_version != BUILD_VERSION && !otaInProgress) {
    doOTA(firmware_url);
  }
}

// Web Handlers
void handleRoot() {
  String html = "<html><head><title>ESP32 Dashboard</title></head><body>";
  html += "<h1>ESP32 Dashboard</h1>";
  html += "<p><strong>Firmware Version:</strong> " BUILD_VERSION "</p>";
  html += "<p><strong>Commit:</strong> " BUILD_COMMIT "</p>";
  html += "<p><strong>Build Date:</strong> " BUILD_DATE "</p>";
  html += "<p><strong>Uptime (ms):</strong> " + String(millis()) + "</p>";
  html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<form action=\"/update\" method=\"post\">";
  html += "<button type=\"submit\">Update Now</button>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  server.send(200, "text/plain", "Manual OTA triggered. Check Serial Monitor...");
  String latest_version, firmware_url;
  if (fetchLatestVersion(latest_version, firmware_url)) {
    if (latest_version != BUILD_VERSION) doOTA(firmware_url);
  }
}

// Setup / Loop
void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();

  lastCheck = millis();
  checkForOTA();
}

void loop() {
  server.handleClient();
  if (millis() - lastCheck > CHECK_INTERVAL) {
    lastCheck = millis();
    checkForOTA();
  }
}
