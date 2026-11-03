#include "pid.h"
#include "config.h"

Pid pid;

void Pid::reset() {
  _integ = 0.0f;
  _prevMeas = NAN;
  _out = 0.0f;
}

float Pid::compute(float sp, float meas) {
  const float dt = 1.0f; // вызов 1 раз в секунду
  const float err = sp - meas;

  // Производная по измерению (без «пинка» при смене уставки)
  float dMeas = isnan(_prevMeas) ? 0.0f : (meas - _prevMeas) / dt;
  _prevMeas = meas;

  float pOut = _kp * err;
  _integ += _ki * err * dt;                       // накопление
  float dOut = -_kd * dMeas;

  float raw = pOut + _integ + dOut;

  // Анти-windup: ограничение интеграла по насыщению выхода
  if (raw > PID_OUT_MAX) { _integ -= (raw - PID_OUT_MAX); raw = PID_OUT_MAX; }
  if (raw < PID_OUT_MIN) { _integ += (PID_OUT_MIN - raw); raw = PID_OUT_MIN; }

  _out = raw;
  return _out;
}

bool Pid::pwmGate(uint32_t now) {
  if ((now - _winStart) >= TTPWM_WINDOW_MS) _winStart = now; // следующее окно
  uint32_t onTime = (uint32_t)(TTPWM_WINDOW_MS * _out / 100.0f);
  return (now - _winStart) < onTime;
}
