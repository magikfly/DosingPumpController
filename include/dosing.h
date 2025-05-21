#pragma once
#include <Arduino.h>

namespace Dosing {
  void setup();
  void loop();
  void calibrate(uint8_t pumpIndex);
  void doseManual(uint8_t pumpIndex, float ml);
  void prime(uint8_t pumpIndex, uint16_t ms);

}
