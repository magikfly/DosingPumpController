#include <time.h>
#include <Arduino.h>
#include <ntp_sync.h>

namespace {
  int syncHour = -1, syncMin = -1;
  time_t lastSyncDay = 0;

  void scheduleNextSync() {
    syncHour = random(0, 24);
    syncMin = random(0, 60);
    Serial.printf("Next NTP sync scheduled at %02d:%02d\n", syncHour, syncMin);
  }
}

void NtpSync::begin(const String& timezone) {
  configTzTime(timezone.c_str(), "192.168.2.254", "time.nist.gov");
  randomSeed(esp_random());
  scheduleNextSync();
  lastSyncDay = 0; // Force first sync
}

void NtpSync::loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  int curHour = timeinfo.tm_hour;
  int curMin = timeinfo.tm_min;
  time_t now = time(NULL);
  int today = timeinfo.tm_yday;

  // Only run once per day at the chosen minute
  if (syncHour == -1 || syncMin == -1) return;
  if (curHour == syncHour && curMin == syncMin && lastSyncDay != today) {
    Serial.println("Performing daily NTP sync...");
    // Re-apply configTzTime to trigger a sync
    configTzTime(nullptr, "192.168.2.254", "time.nist.gov"); // timezone already set
    lastSyncDay = today;
    scheduleNextSync();
  }
}
