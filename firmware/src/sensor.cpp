#include "sensor.h"
#include "config.h"
#include "storage.h"

TempSensor sensor;

void TempSensor::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_NTC_ADC, ADC_11db); // диапазон до ~3.3 В
}

bool TempSensor::readCelsius(float &out) {
  // 8 замеров с усреднением
  uint32_t acc = 0;
  for (int i = 0; i < 8; i++) {
    acc += analogRead(PIN_NTC_ADC);
    delayMicroseconds(200);
  }
  _adc = (int)(acc / 8);

  // Диагностика цепи: V≈0 → обрыв NTC (узел A через R на GND);
  //                   V≈VREF → КЗ NTC (узел A подтянут к 3V3)
  if (_adc < 60)            { _st = SensorStatus::OPEN; return false; }
  if (_adc > ADC_MAX - 60)  { _st = SensorStatus::SHORT; return false; }

  float v   = (float)_adc / ADC_MAX * VREF;
  float r   = DIV_R * (VREF / v - 1.0f);            // сопротивление NTC
  float tK  = 1.0f / (1.0f / 298.15f + logf(r / NTC_R25) / NTC_BETA);
  float tC  = tK - 273.15f + storage.s.calOffset;   // калибровочный сдвиг

  if (tC < -20.0f || tC > 160.0f) { _st = SensorStatus::OUT_OF_RANGE; return false; }

  _st = SensorStatus::OK;
  out = tC;
  return true;
}

void TempSensor::tick() {
  float t;
  if (readCelsius(t)) {
    _tRaw = t;
    _tFilt = isnan(_tFilt) ? t : (_tFilt + 0.4f * (t - _tFilt)); // EMA
  } else {
    _tRaw = NAN;
    _tFilt = NAN; // при отказе показываем NAN — машина перейдёт в ALARM
  }
}

const char* TempSensor::statusStr() const {
  switch (_st) {
    case SensorStatus::OK:           return "OK";
    case SensorStatus::OPEN:         return "NTC OBRYV (open)";
    case SensorStatus::SHORT:        return "NTC KZ (short)";
    default:                         return "OUT OF RANGE";
  }
}
