#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "build_info.h"

// === CONFIG ===
const char* WIFI_SSID = "Ender_2G";
const char* WIFI_PASS = "134679tdg";
const char* VERSION_JSON_URL = "http://enderekici.github.io/esp32c3/version.json";
const unsigned long CHECK_INTERVAL = 5 * 60 * 1000; // 5 minutes

// === GLOBALS ===
WebServer server(80);
unsigned long lastCheck = 0;
String currentVersion = BUILD_VERSION;
bool otaInProgress = false;

// === OTA FUNCTION ===
bool doOTA(const String& firmware_url) {
  if (otaInProgress) return false;
  otaInProgress = true;

  Serial.println("Starting OTA update...");

  HTTPClient http;
  http.begin(firmware_url);
  int code = http.GET();

  if (code != 200) {
    Serial.printf("Failed to fetch firmware. HTTP code: %d\n", code);
    http.end();
    otaInProgress = false;
    return false;
  }

  int len = http.getSize();
  WiFiClient *stream = http.getStreamPtr();

  if (len <= 0) {
    Serial.println("No firmware found at URL");
    http.end();
    otaInProgress = false;
    return false;
  }

  if (!Update.begin(len)) {
    Serial.println("Not enough space for OTA");
    http.end();
    otaInProgress = false;
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
    otaInProgress = false;
    return false;
  }
}

// === FETCH LATEST VERSION INFO ===
bool fetchLatestVersion(String& latest_version, String& firmware_url) {
  HTTPClient http;
  http.begin(VERSION_JSON_URL);
  int code = http.GET();

  if (code != 200) {
    Serial.printf("Failed to fetch version.json. HTTP code: %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println("Failed to parse version.json");
    return false;
  }

  latest_version = doc["version"].as<String>();
  firmware_url = doc["url"].as<String>();
  return true;
}

// === CHECK OTA HELPER ===
void checkForOTA() {
  String latest_version, firmware_url;
  if (!fetchLatestVersion(latest_version, firmware_url)) return;

  Serial.printf("Current version: %s, Latest version: %s\n", currentVersion.c_str(), latest_version.c_str());

  if (latest_version != currentVersion && !otaInProgress) {
    Serial.println("New firmware available. Starting OTA...");
    if (doOTA(firmware_url)) {
      currentVersion = latest_version;
    }
  } else {
    Serial.println("Already up-to-date.");
  }
}

// === DASHBOARD ===
void handleRoot() {
  String html = "<html><head><title>ESP32 Dashboard</title></head><body>";
  html += "<h1>ESP32 Dashboard</h1>";
  html += "<p><strong>Firmware Version:</strong> " + currentVersion + "</p>";
  html += "<p><strong>Commit:</strong> " BUILD_COMMIT "</p>";
  html += "<p><strong>Build Date:</strong> " BUILD_DATE "</p>";
  html += "<p><strong>Uptime (ms):</strong> " + String(millis()) + "</p>";
  html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<form action=\"/update\" method=\"post\"><button type=\"submit\">Update Now</button></form></body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  server.send(200, "text/plain", "Manual OTA triggered. Check Serial Monitor...");
  String latest_version, firmware_url;
  if (fetchLatestVersion(latest_version, firmware_url)) {
    if (latest_version != currentVersion) {
      doOTA(firmware_url);
    } else {
      Serial.println("Already running latest firmware");
    }
  }
}

// === SETUP / LOOP ===
void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  Serial.println("Web server started");

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
