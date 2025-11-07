#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include "src/build_info.h"

const char* WIFI_SSID = "Ender_2G";
const char* WIFI_PASS = "134679tdg";

WebServer server(80);

void handleStatus() {
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"version\":\"" BUILD_VERSION "\",";
  json += "\"commit\":\"" BUILD_COMMIT "\",";
  json += "\"build_date\":\"" BUILD_DATE "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleUpdate() {
  String url = server.arg("url");
  if (!url.length()) {
    server.send(400, "text/plain", "Missing 'url' parameter");
    return;
  }

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != 200) {
    server.send(500, "text/plain", "Download failed");
    return;
  }

  int len = http.getSize();
  WiFiClient *stream = http.getStreamPtr();

  if (!Update.begin(len)) {
    server.send(500, "text/plain", "Not enough space for OTA");
    return;
  }

  size_t written = Update.writeStream(*stream);
  if (Update.end() && Update.isFinished()) {
    server.send(200, "text/plain", "Update successful. Rebooting...");
    delay(1000);
    ESP.restart();
  } else {
    server.send(500, "text/plain", "Update failed.");
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/status", handleStatus);
  server.on("/update", handleUpdate);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
