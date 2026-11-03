#include "machine.h"
#include "config.h"
#include "sensor.h"
#include "pid.h"
#include "storage.h"

Machine machine;

const char* cstateStr(CState s) {
  switch (s) {
    case CState::IDLE:    return "IDLE";
    case CState::HEATING: return "HEATING";
    case CState::HOLD:    return "HOLD";
    case CState::DONE:    return "DONE";
    case CState::ALARM:   return "ALARM";
    default:              return "?";
  }
}

void Machine::begin() {
  pinMode(PIN_SSR, OUTPUT);
  digitalWrite(PIN_SSR, LOW); // безопасное состояние при старте
  if (PIN_BUZZER >= 0) { pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW); }
}

void Machine::start() {
  if (_st == CState::ALARM) return;          // сброс аварии — только stop()
  _st = CState::HEATING;
  _cycle = 0; _remain = storage.s.holdMin * 60;
  _stable = 0; _holdPaused = false; _lastOut = 0;
  _alarm[0] = 0;
  logLen = 0;
  pid.reset();
}

void Machine::stop() {
  _st = CState::IDLE;
  _alarm[0] = 0;
  pid.reset();
}

void Machine::alarm(const char *msg) {
  _st = CState::ALARM;
  strlcpy(_alarm, msg, sizeof(_alarm));
  digitalWrite(PIN_SSR, LOW);
  pid.reset();
}

void Machine::pushLog(float temp) {
  if (logLen >= LOG_CAP) return;
  log[logLen].t = _cycle;
  log[logLen].temp = temp;
  log[logLen].out = (uint8_t)(_lastOut + 0.5f);
  log[logLen].state = (uint8_t)_st;
  logLen++;
}

void Machine::tick() {
  _cycle++;

  if (_st == CState::HEATING || _st == CState::HOLD) {
    // --- Проверки безопасности ---
    if (!sensor.ok()) { alarm(sensor.statusStr()); return; }

    const float t  = sensor.temp();
    const float sp = storage.s.setpoint;

    if (t > HARD_LIMIT)          { alarm("T>125 HARD LIMIT");  return; }
    if (t > sp + OVERSHOOT_LIMIT){ alarm("PEREGREV SP+5");     return; }

    // --- Регулирование ---
    _lastOut = pid.compute(sp, t);

    if (_st == CState::HEATING) {
      if (t >= sp - HOLD_ENTER_DELTA) {
        if (++_stable >= HOLD_ENTER_STABLE) { _st = CState::HOLD; _stable = 0; }
      } else {
        _stable = 0;
      }
      if (_cycle > HEAT_TIMEOUT_S) { alarm("TIMEOUT NAGREVA"); return; }
    } else { // HOLD
      _holdPaused = (t < sp - HOLD_PAUSE_DELTA);
      if (!_holdPaused) {
        if (--_remain <= 0) {
          _st = CState::DONE;
          _doneAtMs = millis();
          pid.reset();
        }
      }
    }

    if ((_cycle % LOG_PERIOD_S) == 0) pushLog(t);
  }
  else if (_st == CState::DONE) {
    // выдержка завершена — нагрев выключен (service() этим управляет)
  }
  // IDLE/ALARM: ничего не делаем
}

void Machine::service() {
  // --- Тепловой выход: только в активных состояниях ---
  const bool heat = (_st == CState::HEATING || _st == CState::HOLD);
  digitalWrite(PIN_SSR, (heat && pid.pwmGate(millis())) ? HIGH : LOW);

  // --- Зуммер (если распаян) ---
  if (PIN_BUZZER >= 0) {
    const uint32_t now = millis();
    bool beep = false;
    if (_st == CState::DONE  && (now - _doneAtMs) < 6000) beep = ((now / 300) % 2) == 0; // трель 6 с
    if (_st == CState::ALARM)                            beep = ((now / 500) % 2) == 0; // прерывистый
    digitalWrite(PIN_BUZZER, beep ? HIGH : LOW);
  }
}
