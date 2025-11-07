#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "build_info.h"

const char* WIFI_SSID = "Ender_2G";
const char* WIFI_PASS = "134679tdg";
const char* VERSION_JSON_URL = "https://enderekici.github.io/esp32c3/version.json";
const unsigned long CHECK_INTERVAL = 5 * 60 * 1000;

WebServer server(80);
unsigned long lastCheck = 0;

struct Firmware {
  String name;
  String version;
  String url;
};

bool doOTA(const String& firmware_url) {
  Serial.println("Starting OTA update...");
  HTTPClient http;
  http.begin(firmware_url);
  int code = http.GET();

  if (code != 200) { Serial.printf("Failed to fetch firmware. HTTP code: %d\n", code); http.end(); return false; }

  int len = http.getSize();
  WiFiClient *stream = http.getStreamPtr();
  if (len <= 0) { Serial.println("No firmware found at URL"); http.end(); return false; }
  if (!Update.begin(len)) { Serial.println("Not enough space for OTA"); http.end(); return false; }

  size_t written = Update.writeStream(*stream);
  if (Update.end() && Update.isFinished()) { Serial.println("OTA update complete! Rebooting..."); delay(1000); ESP.restart(); return true; }
  else { Serial.println("OTA update failed"); http.end(); return false; }
}

bool fetchFirmwares(Firmware firmware_list[], int& count) {
  HTTPClient http;
  http.begin(VERSION_JSON_URL);
  int code = http.GET();
  if (code != 200) { Serial.printf("Failed to fetch version.json. HTTP code: %d\n", code); http.end(); return false; }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) { Serial.println("Failed to parse version.json"); return false; }

  JsonArray arr = doc["firmwares"].as<JsonArray>();
  count = arr.size();
  for (int i = 0; i < count; i++) {
    firmware_list[i].name = arr[i]["name"].as<String>();
    firmware_list[i].version = arr[i]["version"].as<String>();
    firmware_list[i].url = arr[i]["url"].as<String>();
  }
  return true;
}

void checkForOTA() {
  Firmware firmware_list[10];
  int count = 0;
  if (!fetchFirmwares(firmware_list, count)) {
    Serial.println("Failed to fetch firmware list.");
    return;
  }

  for (int i = 0; i < count; i++) {
    Firmware f = firmware_list[i];
    if (f.name == "dashboard" && f.version != BUILD_VERSION) {
      Serial.printf("New firmware available for %s: %s\n", f.name.c_str(), f.version.c_str());
      doOTA(f.url);
    } else {
      Serial.printf("%s is up-to-date.\n", f.name.c_str());
    }
  }
}

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
  checkForOTA();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  Serial.println("\nConnected!");
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  Serial.println("Web server started");

  lastCheck = millis();
  checkForOTA();
}

void loop() {
  server.handleClient();
  if (millis() - lastCheck > CHECK_INTERVAL) { lastCheck = millis(); checkForOTA(); }
}
