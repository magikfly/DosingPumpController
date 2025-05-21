#pragma once

#include <Arduino.h>

namespace NtpSync {
  void begin(const String& timezone);
  void loop();
}