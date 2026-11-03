#pragma once
// ============================================================
//  MCAC v0.1 — конфигурация железа и параметров
//  ESP32 DevKit (WROOM-32) + LCD1602(I2C) + SSR-25DA(2N7000) + NTC
// ============================================================

// ---------- Пины ----------
constexpr int PIN_SSR      = 25;   // на затвор 2N7000 через 150R (см. docs/ASSEMBLY.md)
constexpr int PIN_NTC_ADC  = 34;   // ADC1_CH6, input-only: середина делителя NTC
constexpr int PIN_BUZZER   = 26;   // опц.: активный зуммер через NPN; -1 если нет
constexpr int PIN_I2C_SDA  = 21;
constexpr int PIN_I2C_SCL  = 22;
constexpr uint8_t LCD_ADDR = 0x27; // если экран не отвечает — попробовать 0x3F
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;

// ---------- Датчик: штатный NTC (топология: 3V3 — NTC — [узел A] — R_DIV — GND) ----------
constexpr float NTC_R25  = 100000.0f; // Ом @25°C — ЗАМЕНИТЬ на замеренное R25 своего датчика!
constexpr float NTC_BETA = 3950.0f;   // B-коэффициент; уточняется калибровкой (ASSEMBLY §8.3)
constexpr float DIV_R    = 10000.0f;  // Ом, резистор делителя 1% (10к при R25=100к)
constexpr float VREF     = 3.3f;
constexpr int   ADC_MAX  = 4095;

// ---------- Цикл стерилизации ----------
constexpr float DEF_SETPOINT      = 121.0f;  // °C
constexpr int   DEF_HOLD_MIN      = 20;      // минут выдержки
constexpr float SETPOINT_MIN      = 40.0f;   // пределы уставки из веб-UI
constexpr float SETPOINT_MAX      = 123.0f;
constexpr float HOLD_ENTER_DELTA  = 0.5f;    // вход в HOLD: |T − SP| ≤ Δ
constexpr int   HOLD_ENTER_STABLE = 30;      // …секунд стабильности подряд
constexpr float HOLD_PAUSE_DELTA  = 2.0f;    // пауза выдержки при T < SP − Δ
constexpr float OVERSHOOT_LIMIT   = 5.0f;    // авария: T > SP + 5
constexpr float HARD_LIMIT        = 125.0f;  // авария: T > 125 (программный предел)
constexpr uint32_t HEAT_TIMEOUT_S = 120UL * 60UL; // авария: нагрев дольше 2 ч
constexpr uint32_t LOG_PERIOD_S   = 5;       // период записи в журнал

// ---------- PID / время-пропорциональный выход (TPPWM) ----------
constexpr float PID_KP = 10.0f;    // % мощности на °C
constexpr float PID_KI = 0.15f;    // % на (°C·с)
constexpr float PID_KD = 30.0f;    // %·с на °C
constexpr uint32_t TTPWM_WINDOW_MS = 8000; // окно модуляции мощности
constexpr float PID_OUT_MAX = 100.0f;
constexpr float PID_OUT_MIN = 0.0f;

// ---------- Wi-Fi / сеть ----------
constexpr const char* AP_SSID   = "MCAC-Setup"; // точка доступа при отсутствии настроек
constexpr const char* HOSTNAME  = "mcac";       // http://mcac.local

// ---------- Версия ----------
constexpr const char* FW_VERSION = "0.1.0";
