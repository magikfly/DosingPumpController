#include <Arduino.h>
#include <SPIFFS.h>
#include "wi_fi.h"
#include "storage.h"
#include "ntp_sync.h"
#include "ui.h"
#include "dosing.h"

void setup() {
  Serial.begin(115200);
  Serial.println("delaying");
  delay(10000);
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    while (true) delay(1000);
  }
  Serial.println("SPIFFS mounted");
  Storage::begin();
  WiFiManager::connect();
  NtpSync::begin(Storage::getTimezone());
  setupUI();       // <-- ALL ROUTES AND server.begin() here!
  //PumpUI::setup();
  //SettingsUI::setup();
  Dosing::setup();
}

void loop() {
  Dosing::loop();
  NtpSync::loop();
}
