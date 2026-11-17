#include "sensor.h"
#include "config.h"
#include "storage.h"

TempSensor sensor;

void TempSensor::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_NTC_ADC, ADC_11db); // диапазон до ~3.3 В
}

bool TempSensor::readCelsius(float &out) {
  // 64 замера с усреднением: trimmean — отбрасываем по 4 мин. и 4 макс.
  // (устойчиво к выбросам ADC при Wi-Fi-транзитах и пульсациях шины 3V3)
  const int N = 64, CUT = 4;
  uint16_t buf[N];
  for (int i = 0; i < N; i++) {
    buf[i] = (uint16_t)analogRead(PIN_NTC_ADC);
    delayMicroseconds(1200);                       // ~77 мс на полный замер
  }
  // сортировка вставками (N=64 — дёшево)
  for (int i = 1; i < N; i++) {
    uint16_t v = buf[i];
    int j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j + 1] = buf[j]; j--; }
    buf[j + 1] = v; // вставка на позицию после последнего сдвига
  }
  uint32_t acc = 0;
  for (int i = CUT; i < N - CUT; i++) acc += buf[i];
  _adc = (int)(acc / (N - 2 * CUT));

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
