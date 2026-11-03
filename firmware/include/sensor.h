#pragma once
#include <Arduino.h>

enum class SensorStatus : uint8_t { OK = 0, OPEN, SHORT, OUT_OF_RANGE };

// Датчик температуры на NTC (делитель 3V3—NTC—[ADC]—R—GND)
class TempSensor {
public:
  void begin();                 // настройка ADC
  void tick();                  // вызывать 1 раз в секунду
  float temp() const { return _tFilt; }   // фильтрованная, °C (NAN при отказе)
  float tempRaw() const { return _tRaw; } // без фильтра, °C
  int   adcRaw() const { return _adc; }
  SensorStatus status() const { return _st; }
  bool  ok() const { return _st == SensorStatus::OK; }
  const char* statusStr() const;
private:
  bool readCelsius(float &out);
  int   _adc = 0;
  float _tRaw = NAN, _tFilt = NAN;
  SensorStatus _st = SensorStatus::OUT_OF_RANGE;
};

extern TempSensor sensor;
