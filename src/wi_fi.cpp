
#include "wi_fi.h"
#include <WiFi.h>
#include "storage.h"

void WiFiManager::connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("wifi", "pass");
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}


