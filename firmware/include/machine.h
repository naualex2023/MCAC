#pragma once
#include <Arduino.h>

enum class CState : uint8_t { IDLE = 0, HEATING, HOLD, DONE, ALARM };
const char* cstateStr(CState s);

struct CycleLogEntry {
  uint32_t t;      // сек от начала цикла
  float    temp;   // °C
  uint8_t  out;    // мощность, %
  uint8_t  state;  // CState
};

// Конечный автомат цикла стерилизации
class Machine {
public:
  void begin();
  void tick();     // вызывать 1 раз в секунду (регулирование, аварии, журнал)
  void service();  // вызывать каждый проход loop (TPPWM на SSR, зуммер)

  CState  state() const { return _st; }
  const char* alarmMsg() const { return _alarm; }
  uint32_t cycleSec() const { return _cycle; }
  int  remainSec() const { return _remain; }
  bool holdPaused() const { return _holdPaused; }
  float lastPower() const { return _lastOut; }

  void start();
  void stop();     // из любого состояния, включая сброс аварии

  // Журнал текущего цикла (кольцевой, в RAM)
  static constexpr size_t LOG_CAP = 720;   // 720×5с = 1 час
  CycleLogEntry log[LOG_CAP];
  size_t logLen = 0;

private:
  void alarm(const char *msg);
  void pushLog(float temp);

  CState   _st = CState::IDLE;
  char     _alarm[33] = {0};
  uint32_t _cycle = 0;          // сек с начала цикла
  int      _remain = 0;         // сек до конца выдержки
  int      _stable = 0;         // сек стабильности для входа в HOLD
  bool     _holdPaused = false;
  float    _lastOut = 0.0f;
  uint32_t _doneAtMs = 0;
};

extern Machine machine;
