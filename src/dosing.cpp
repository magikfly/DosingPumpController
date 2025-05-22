#include "dosing.h"
#include "storage.h"
#include <Arduino.h>
#include <time.h>

const uint8_t pumpPins[NUM_PUMPS] = {16, 17, 18}; // You can change these as needed

struct PumpState {
  bool running = false;
  unsigned long endMillis = 0;
  float targetMl = 0; // For updating remaining volume when stopped
};
static PumpState pumpStates[NUM_PUMPS];

void startPump(uint8_t pumpIndex, float ml) {
  float mlps = Storage::getCalibration(pumpIndex);
  if (mlps <= 0) mlps = 1.0;
  unsigned long ms = (unsigned long)((ml / mlps) * 1000.0);
  digitalWrite(pumpPins[pumpIndex], HIGH);
  pumpStates[pumpIndex].running = true;
  pumpStates[pumpIndex].endMillis = millis() + ms;
  pumpStates[pumpIndex].targetMl = ml;
}

void stopPump(uint8_t pumpIndex) {
  digitalWrite(pumpPins[pumpIndex], LOW);
  pumpStates[pumpIndex].running = false;
  // Only subtract if a dose was actually requested (ignore for prime)
  if (pumpStates[pumpIndex].targetMl > 0) {
    float rem = Storage::getRemainingVolume(pumpIndex) - pumpStates[pumpIndex].targetMl;
    Storage::setRemainingVolume(pumpIndex, rem < 0 ? 0 : rem);
  }
  pumpStates[pumpIndex].targetMl = 0;
}

void Dosing::setup() {
  for (uint8_t i = 0; i < NUM_PUMPS; ++i) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], LOW);
    pumpStates[i].running = false;
    pumpStates[i].targetMl = 0;
  }
}

void Dosing::loop() {
  unsigned long now = millis();
  // Stop pumps if finished
  for (uint8_t i = 0; i < NUM_PUMPS; ++i) {
    if (pumpStates[i].running && now >= pumpStates[i].endMillis) {
      stopPump(i);
    }
  }

  // Scheduled dosing logic
  time_t tnow = time(nullptr);
  struct tm *tmNow = localtime(&tnow);
  uint16_t minsNow = tmNow->tm_hour * 60 + tmNow->tm_min;
  uint8_t today = tmNow->tm_mday;

  static uint8_t lastDosedDay[NUM_PUMPS][MAX_DOSES_PER_DAY] = {{0}};

  for (uint8_t p = 0; p < NUM_PUMPS; ++p) {
    uint8_t daysMask = Storage::getPumpActiveDays(p);
    if (!Storage::isPumpEnabled(p)) continue;
    if (!(daysMask & (1 << tmNow->tm_wday))) continue;
    uint8_t doses = Storage::getDosesPerDay(p);
    // For each scheduled dose
    for (uint8_t d = 0; d < doses; ++d) {
      uint16_t schedMin = Storage::getDoseTime(p, d);
      if (minsNow == schedMin && lastDosedDay[p][d] != today) {
        // Use per-dose amount if present, else fall back to dailyTotal/doses
        float amt = Storage::getDoseAmount(p, d);
        if (amt <= 0.0f) {
          float dailyTotal = Storage::getDailyDose(p);
          amt = dailyTotal / doses;
        }
        startPump(p, amt);
        lastDosedDay[p][d] = today;
        // Optionally: add dose history here
        // Storage::addDoseHistory(p, amt, "auto", ...);
      }
    }
  }
}



void Dosing::calibrate(uint8_t pumpIndex) {
  // Calibration runs are handled via UI and corresponding handler calls startPump
}

void Dosing::doseManual(uint8_t pumpIndex, float ml) {
  startPump(pumpIndex, ml);
}

void Dosing::primeStart(uint8_t pumpIndex) {
  if (pumpIndex >= NUM_PUMPS) return;
  digitalWrite(pumpPins[pumpIndex], HIGH);
  pumpStates[pumpIndex].running = true;
  pumpStates[pumpIndex].endMillis = 0;
  pumpStates[pumpIndex].targetMl = 0;
}

void Dosing::primeStop(uint8_t pumpIndex) {
  if (pumpIndex >= NUM_PUMPS) return;
  digitalWrite(pumpPins[pumpIndex], LOW);
  pumpStates[pumpIndex].running = false;
  pumpStates[pumpIndex].endMillis = 0;
  pumpStates[pumpIndex].targetMl = 0;
}

// --- Keep this for calibration or fixed-time actions ---
void Dosing::prime(uint8_t pumpIndex, uint16_t ms) {
  digitalWrite(pumpPins[pumpIndex], HIGH);
  pumpStates[pumpIndex].running = true;
  pumpStates[pumpIndex].endMillis = millis() + ms;
  pumpStates[pumpIndex].targetMl = 0; // No volume update for priming
}
