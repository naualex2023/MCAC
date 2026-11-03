#include "ui_lcd.h"
#include "config.h"
#include "sensor.h"
#include "machine.h"
#include "storage.h"

UiLcd ui;

void UiLcd::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  _lcd.init();
  _lcd.backlight();
  _lcd.setCursor(0, 0); _lcd.print("MCAC v" FW_VERSION);
  _lcd.setCursor(0, 1); _lcd.print("Sterilizer ready");
  _ok = true;
}

static void fmtTime(char *buf, size_t n, uint32_t sec) {
  uint32_t m = sec / 60, s = sec % 60;
  snprintf(buf, n, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

void UiLcd::update() {
  if (!_ok) return;

  char l0[17], l1[17], tt[8];

  // Строка 0: температура и уставка
  if (isnan(sensor.temp()))
    snprintf(l0, sizeof(l0), "T=--.- %s", sensor.statusStr());
  else
    snprintf(l0, sizeof(l0), "T=%5.1fC SP:%3.0f", (double)sensor.temp(), (double)storage.s.setpoint);

  // Строка 1: состояние и время (все строки ≤ 16 символов)
  switch (machine.state()) {
    case CState::IDLE:
      snprintf(l1, sizeof(l1), "IDLE hold %3dmin", storage.s.holdMin);
      break;
    case CState::HEATING:
      fmtTime(tt, sizeof(tt), machine.cycleSec());
      snprintf(l1, sizeof(l1), "HEAT %s P%3.0f", tt, (double)machine.lastPower());
      break;
    case CState::HOLD:
      fmtTime(tt, sizeof(tt), (uint32_t)machine.remainSec());
      snprintf(l1, sizeof(l1), "%s %s P%3.0f", machine.holdPaused() ? "PAUSE" : "HOLD ", tt, (double)machine.lastPower());
      break;
    case CState::DONE:
      snprintf(l1, sizeof(l1), "DONE! cycle ok");
      break;
    case CState::ALARM:
      snprintf(l1, sizeof(l1), "ALARM %.10s", machine.alarmMsg());
      break;
    default:
      snprintf(l1, sizeof(l1), "?");
  }

  _lcd.setCursor(0, 0); _lcd.print(l0);
  _lcd.setCursor(0, 1); _lcd.print(l1);
}
