#pragma once
#include <Arduino.h>

enum class SensorStatus : uint8_t { OK = 0, OPEN, SHORT, OUT_OF_RANGE, OPEN_OR_SHORT, DIV_FAULT };

// Датчик температуры на NTC, делитель. Топология — NTC_TOPOLOGY в config.h:
//   0: 3V3—NTC—[узел A]—R_DIV—GND   (обрыв/КЗ неразличимы — совместимость)
//   1: 3V3—R_DIV—[узел A]—NTC—GND   (обрыв→adc≈VREF, КЗ→adc≈0)
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
