#include "sensor.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 по 1-Wire: цифровая шина вместо делителя/ADC.
// Конверсия асинхронная: tick() стартует преобразование, читает результат
// на следующем такте (через >750 мс) — loop не блокируется.
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature ds(&oneWire);

TempSensor sensor;

void TempSensor::begin() {
  ds.begin();
  if (ds.getDeviceCount() == 0) {
    _st = SensorStatus::OPEN;
    return;
  }
  ds.setResolution(DS_RES_BITS);
  ds.setWaitForConversion(false); // асинхронный режим
  request();
}

void TempSensor::request() {
  ds.requestTemperatures();
  _reqMs = millis();
  _pending = true;
}

void TempSensor::tick() {
  if (!_pending) {
    request();
    return;
  }
  if (millis() - _reqMs < DS_CONV_MS)
    return;                       // конверсия ещё идёт
  _pending = false;

  float t = ds.getTempCByIndex(0);
  // DEVICE_DISCONNECTED_C = -127: нет датчика на шине или CRC-ошибка
  if (t <= -100.0f) {
    _st = SensorStatus::OPEN;
    _tRaw = NAN;
    _tFilt = NAN;                 // машина перейдёт в ALARM
    return;
  }
  if (t < -20.0f || t > 160.0f) {
    _st = SensorStatus::OUT_OF_RANGE;
    _tRaw = NAN;
    _tFilt = NAN;
    return;
  }
  _st = SensorStatus::OK;
  _tRaw = t;
  _rawCode = (int)(t * 16.0f);    // сырой 12-битный код датчика (°C×16)
  _tFilt = isnan(_tFilt) ? t : (_tFilt + 0.4f * (t - _tFilt)); // EMA как в NTC-версии
}

const char* TempSensor::statusStr() const {
  switch (_st) {
    case SensorStatus::OK:  return "OK";
    case SensorStatus::OPEN: return "DS18B20 NET (no dev/CRC)";
    default: return "OUT OF RANGE";
  }
}
