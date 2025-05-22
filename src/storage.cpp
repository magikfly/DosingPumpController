#include "storage.h"
#include <vector>

static Preferences prefs;
struct DoseEntry { float amount; String type; String time; };
std::vector<DoseEntry> doseHistories[NUM_PUMPS];

void Storage::begin() {
  prefs.begin("pumpcfg", false);
  for (uint8_t i = 0; i < NUM_PUMPS; ++i) {
    String key = "doseday_" + String(i);
    if (!prefs.isKey(key.c_str())) {
      prefs.putFloat(key.c_str(), 30.0); // or whatever default you prefer
    }
    // Do this for other important defaults too
  }
}

String Storage::getPumpName(uint8_t pumpIndex) {
  return prefs.getString(("pumpname_" + String(pumpIndex)).c_str(), "Pump " + String(pumpIndex+1));
}
void Storage::setPumpName(uint8_t pumpIndex, const String &name) {
  prefs.putString(("pumpname_" + String(pumpIndex)).c_str(), name);
}

float Storage::getCalibration(uint8_t pumpIndex) {
  return prefs.getFloat(("mlps_" + String(pumpIndex)).c_str(), 1.0);
}
void Storage::setCalibration(uint8_t pumpIndex, float mlps) {
  prefs.putFloat(("mlps_" + String(pumpIndex)).c_str(), mlps);
}

float Storage::getContainerVolume(uint8_t pumpIndex) {
  return prefs.getFloat(("contvol_" + String(pumpIndex)).c_str(), 1000.0);
}
void Storage::setContainerVolume(uint8_t pumpIndex, float volume) {
  prefs.putFloat(("contvol_" + String(pumpIndex)).c_str(), volume);
}

float Storage::getRemainingVolume(uint8_t pumpIndex) {
  return prefs.getFloat(("remvol_" + String(pumpIndex)).c_str(), getContainerVolume(pumpIndex));
}
void Storage::setRemainingVolume(uint8_t pumpIndex, float volume) {
  prefs.putFloat(("remvol_" + String(pumpIndex)).c_str(), volume);
}

uint8_t Storage::getDosesPerDay(uint8_t pumpIndex) {
  return prefs.getUChar(("doses_" + String(pumpIndex)).c_str(), 3);
}
void Storage::setDosesPerDay(uint8_t pumpIndex, uint8_t doses) {
  prefs.putUChar(("doses_" + String(pumpIndex)).c_str(), doses);
}

float Storage::getDailyDose(uint8_t pumpIndex) {
  return prefs.getFloat(("doseday_" + String(pumpIndex)).c_str(), 30.0);
}
void Storage::setDailyDose(uint8_t pumpIndex, float ml) {
  prefs.putFloat(("doseday_" + String(pumpIndex)).c_str(), ml);
}

uint16_t Storage::getDoseTime(uint8_t pumpIndex, uint8_t doseIndex) {
  String key = "dosetime_" + String(pumpIndex) + "_" + String(doseIndex);
  uint8_t doses = getDosesPerDay(pumpIndex);
  if (doses < 1) doses = 1;
  return prefs.getUShort(key.c_str(), (doseIndex * 1440) / doses); // default: evenly spread
}
void Storage::setDoseTime(uint8_t pumpIndex, uint8_t doseIndex, uint16_t minsAfterMidnight) {
  String key = "dosetime_" + String(pumpIndex) + "_" + String(doseIndex);
  prefs.putUShort(key.c_str(), minsAfterMidnight);
}

// WiFi/Timezone
String Storage::getWiFiSSID() { return prefs.getString("wifi_ssid", ""); }
String Storage::getWiFiPassword() { return prefs.getString("wifi_pass", ""); }
void Storage::setWiFiCredentials(const String &ssid, const String &pass) {
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", pass);
}
String Storage::getTimezone() { return prefs.getString("timezone", "UTC"); }
void Storage::setTimezone(const String &tz) { prefs.putString("timezone", tz); }

bool Storage::isPumpEnabled(uint8_t pumpIndex) {
    return prefs.getBool(("enabled_" + String(pumpIndex)).c_str(), true); // Default: enabled
}

void Storage::setPumpEnabled(uint8_t pumpIndex, bool enabled) {
    prefs.putBool(("enabled_" + String(pumpIndex)).c_str(), enabled);
}

String Storage::getDoseLabel(uint8_t pump, uint8_t dose) {
    return prefs.getString(("doseLabel_" + String(pump) + "_" + String(dose)).c_str(), "");
}
void Storage::setDoseLabel(uint8_t pump, uint8_t dose, const String &label) {
    prefs.putString(("doseLabel_" + String(pump) + "_" + String(dose)).c_str(), label);
}

void Storage::addDoseHistory(uint8_t pump, float amount, const String &type, const String &time) {
    if (pump >= NUM_PUMPS) return;
    doseHistories[pump].push_back({amount, type, time});
    if (doseHistories[pump].size() > 20) doseHistories[pump].erase(doseHistories[pump].begin());
}
void Storage::getDoseHistory(uint8_t pump, JsonArray arr) {
    if (pump >= NUM_PUMPS) return;
    for (auto &h : doseHistories[pump]) {
        JsonObject obj = arr.createNestedObject();
        obj["amount"] = h.amount;
        obj["type"] = h.type;
        obj["time"] = h.time;
    }
}

float Storage::getDoseAmount(uint8_t pump, uint8_t dose) {
    return prefs.getFloat(("doseAmt_" + String(pump) + "_" + String(dose)).c_str(), 0.0);
}
void Storage::setDoseAmount(uint8_t pump, uint8_t dose, float amount) {
    prefs.putFloat(("doseAmt_" + String(pump) + "_" + String(dose)).c_str(), amount);
}

uint8_t Storage::getPumpActiveDays(uint8_t pump) {
    return prefs.getUChar(("activeDays_" + String(pump)).c_str(), 0b01111110); // Default: Mon-Fri
}
void Storage::setPumpActiveDays(uint8_t pump, uint8_t days) {
    prefs.putUChar(("activeDays_" + String(pump)).c_str(), days);
}

void Storage::saveAll() {
  prefs.end();
  prefs.begin("pumpcfg", false);
}
