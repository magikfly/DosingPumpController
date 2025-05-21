#include "dosing.h"
#include "storage.h"
#include <Arduino.h>
#include <time.h>

// Example pump pins for D1 Mini ESP32 (customize as needed)
const uint8_t pumpPins[NUM_PUMPS] = {16, 17, 18};

void runPump(uint8_t pumpIndex, float ml) {
  float mlps = Storage::getCalibration(pumpIndex);
  unsigned long ms = (unsigned long)((ml / mlps) * 1000.0);
  digitalWrite(pumpPins[pumpIndex], HIGH);
  delay(ms);
  digitalWrite(pumpPins[pumpIndex], LOW);
  // Update remaining volume
  float rem = Storage::getRemainingVolume(pumpIndex) - ml;
  Storage::setRemainingVolume(pumpIndex, rem < 0 ? 0 : rem);
}

void Dosing::setup() {
  for (uint8_t i = 0; i < NUM_PUMPS; ++i) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], LOW);
  }
}

void Dosing::loop() {
  // Scheduled dosing logic
  time_t now = time(nullptr);
  struct tm *tmNow = localtime(&now);
  uint16_t minsNow = tmNow->tm_hour * 60 + tmNow->tm_min;
  uint8_t today = tmNow->tm_mday;

  static uint8_t lastDosedDay[NUM_PUMPS][MAX_DOSES_PER_DAY] = {{0}};

  for (uint8_t p = 0; p < NUM_PUMPS; ++p) {
    if (!Storage::isPumpEnabled(p)) continue;
    uint8_t doses = Storage::getDosesPerDay(p);
    float dailyTotal = Storage::getDailyDose(p);
    float perDose = dailyTotal / doses;
    for (uint8_t d = 0; d < doses; ++d) {
      uint16_t schedMin = Storage::getDoseTime(p, d);
      if (minsNow == schedMin && lastDosedDay[p][d] != today) {
        runPump(p, perDose);
        lastDosedDay[p][d] = today;
      }
    }
  }
}

void Dosing::calibrate(uint8_t pumpIndex) {
  // This function would start a calibration run, user measures output, then sets value via webUI
  // The web UI should POST measured ml/sec to Storage::setCalibration()
}

void Dosing::doseManual(uint8_t pumpIndex, float ml) {
  runPump(pumpIndex, ml);
}

void Dosing::prime(uint8_t pumpIndex, uint16_t ms) {
  digitalWrite(pumpPins[pumpIndex], HIGH);
  delay(ms);
  digitalWrite(pumpPins[pumpIndex], LOW);
}
