#include "storage.h"

Storage storage;

void Storage::begin() {
  _prefs.begin("mcac", false);
}

void Storage::load() {
  s.setpoint  = _prefs.getFloat("sp",   DEF_SETPOINT);
  s.holdMin   = _prefs.getInt("hold",   DEF_HOLD_MIN);
  s.calOffset = _prefs.getFloat("cal",  0.0f);
  s.kp        = _prefs.getFloat("kp",   PID_KP);
  s.ki        = _prefs.getFloat("ki",   PID_KI);
  s.kd        = _prefs.getFloat("kd",   PID_KD);
  String ssid = _prefs.getString("ssid", "");
  String pass = _prefs.getString("pass", "");
  strlcpy(s.wifiSsid, ssid.c_str(), sizeof(s.wifiSsid));
  strlcpy(s.wifiPass, pass.c_str(), sizeof(s.wifiPass));
}

void Storage::save() {
  _prefs.putFloat("sp",   s.setpoint);
  _prefs.putInt("hold",   s.holdMin);
  _prefs.putFloat("cal",  s.calOffset);
  _prefs.putFloat("kp",   s.kp);
  _prefs.putFloat("ki",   s.ki);
  _prefs.putFloat("kd",   s.kd);
  _prefs.putString("ssid", String(s.wifiSsid));
  _prefs.putString("pass", String(s.wifiPass));
}
