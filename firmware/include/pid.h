#pragma once
#include <Arduino.h>
#include "config.h"

// ПИД-регулятор с время-пропорциональным выходом (TPPWM) для SSR
class Pid {
public:
  void setGains(float kp, float ki, float kd) { _kp = kp; _ki = ki; _kd = kd; }
  void reset();                              // сброс интеграла/истории
  float compute(float setpoint, float meas); // вызов 1 раз/с → мощность 0..100 %
  float output() const { return _out; }
  bool  pwmGate(uint32_t nowMs);             // включён ли SSR в данный момент
private:
  float _kp = PID_KP, _ki = PID_KI, _kd = PID_KD;
  float _integ = 0.0f, _prevMeas = NAN, _out = 0.0f;
  uint32_t _winStart = 0;
};

extern Pid pid;
