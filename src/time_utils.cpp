#include "time_utils.h"
#include <time.h>

static long tzOffset = 0; // seconds

namespace TimeUtils {
  void begin(long timezoneOffsetSec) {
    tzOffset = timezoneOffsetSec;
    configTime(tzOffset, 0, "pool.ntp.org", "time.nist.gov");
  }

  unsigned long getEpoch() {
    time_t now;
    time(&now);
    return (unsigned long)now + tzOffset;
  }

  void setTimezoneOffset(long offsetSec) {
    tzOffset = offsetSec;
    configTime(tzOffset, 0, "pool.ntp.org", "time.nist.gov");
  }

  long getTimezoneOffset() {
    return tzOffset;
  }
}
