#pragma once
#include <Arduino.h>

enum class SensorStatus : uint8_t { OK = 0, OPEN, OUT_OF_RANGE };

// Датчик температуры DS18B20 по 1-Wire (ветка feature/ds18b20).
// Замена NTC-версии из main: без делителя/ADC, точность заводская ±0.5°C (до 85°C).
class TempSensor {
public:
  void begin();                 // инициализация шины 1-Wire
  void tick();                  // вызывать 1 раз в секунду (async: запрос→чтение)
  float temp() const { return _tFilt; }   // фильтрованная, °C (NAN при отказе)
  float tempRaw() const { return _tRaw; } // без фильтра, °C
  int   adcRaw() const { return _rawCode; } // сырой код DS18B20 (°C×16) — отладка
  SensorStatus status() const { return _st; }
  bool  ok() const { return _st == SensorStatus::OK; }
  const char* statusStr() const;
private:
  void request();
  int   _rawCode = 0;
  float _tRaw = NAN, _tFilt = NAN;
  SensorStatus _st = SensorStatus::OUT_OF_RANGE;
  uint32_t _reqMs = 0;
  bool    _pending = false;
};

extern TempSensor sensor;
