#include "ntp_sync.h"
#include <time.h>
#include <lwip/apps/sntp.h>

void NtpSync::begin(const String& timezone) {
  configTzTime(timezone.c_str(), "pool.ntp.org", "time.nist.gov");
}

void NtpSync::loop() {
  // No-op for now, can refresh NTP if needed periodically
}