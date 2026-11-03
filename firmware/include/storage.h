#pragma once
#include <Preferences.h>
#include "config.h"

// Настройки, сохраняемые во флеш (NVS)
struct Settings {
  float setpoint  = DEF_SETPOINT; // °C
  int   holdMin   = DEF_HOLD_MIN; // минут
  float calOffset = 0.0f;         // °C, калибровочный сдвиг датчика
  float kp = PID_KP, ki = PID_KI, kd = PID_KD;
  char  wifiSsid[33] = {0};
  char  wifiPass[65] = {0};
};

class Storage {
public:
  void begin();
  void load();
  void save();
  Settings s;
private:
  Preferences _prefs;
};

extern Storage storage;
