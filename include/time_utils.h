#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

namespace TimeUtils {
  void begin(long timezoneOffsetSec);
  unsigned long getEpoch();
  void setTimezoneOffset(long offsetSec);
  long getTimezoneOffset();
}

#endif
