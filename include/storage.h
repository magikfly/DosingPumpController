#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define NUM_PUMPS 3
#define MAX_DOSES_PER_DAY 24

namespace Storage {
  void begin();

  // Pump naming
  String getPumpName(uint8_t pumpIndex);
  void setPumpName(uint8_t pumpIndex, const String &name);

  bool isPumpEnabled(uint8_t pumpIndex);
  void setPumpEnabled(uint8_t pumpIndex, bool enabled);

  // Per-pump config
  float getCalibration(uint8_t pumpIndex);
  void setCalibration(uint8_t pumpIndex, float mlps);

  float getContainerVolume(uint8_t pumpIndex);
  void setContainerVolume(uint8_t pumpIndex, float volume);

  float getRemainingVolume(uint8_t pumpIndex);
  void setRemainingVolume(uint8_t pumpIndex, float volume);

  uint8_t getDosesPerDay(uint8_t pumpIndex);
  void setDosesPerDay(uint8_t pumpIndex, uint8_t doses);

  float getDailyDose(uint8_t pumpIndex);
  void setDailyDose(uint8_t pumpIndex, float ml);

  // Custom times (minutes after midnight)
  uint16_t getDoseTime(uint8_t pumpIndex, uint8_t doseIndex);
  void setDoseTime(uint8_t pumpIndex, uint8_t doseIndex, uint16_t minsAfterMidnight);

  String getDoseLabel(uint8_t pump, uint8_t dose);
  void setDoseLabel(uint8_t pump, uint8_t dose, const String &label);
  // If you want to store history:
  void addDoseHistory(uint8_t pump, float amount, const String &type, const String &time);
  void getDoseHistory(uint8_t pump, JsonArray arr);

  float getDoseAmount(uint8_t pump, uint8_t dose);
  void setDoseAmount(uint8_t pump, uint8_t dose, float amount);

  // WiFi/Timezone
  String getWiFiSSID();
  String getWiFiPassword();
  void setWiFiCredentials(const String &ssid, const String &pass);

  String getTimezone();
  void setTimezone(const String &tz);

  void saveAll();
}
