#include "ui_lcd.h"
#include "config.h"
#include "sensor.h"
#include "machine.h"
#include "storage.h"
#include "webui.h"
#include <WiFi.h>

UiLcd ui;

void UiLcd::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  _lcd.init();
  _lcd.backlight();
  _lcd.setCursor(0, 0); _lcd.print("MCAC v" FW_VERSION);
  _lcd.setCursor(0, 1); _lcd.print("Sterilizer ready");
  if (LCD_ROWS >= 4) {
    _lcd.setCursor(0, 2); _lcd.print("waiting for WiFi");
    _lcd.setCursor(0, 3); _lcd.print("...");
  }
  _ok = true;
}

static void fmtTime(char *buf, size_t n, uint32_t sec) {
  uint32_t m = sec / 60, s = sec % 60;
  snprintf(buf, n, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

void UiLcd::update() {
  if (!_ok) return;

  const bool wide = (LCD_COLS >= 20); // 20×4 или 16×2
  char l0[21], l1[21], tt[8];

  // ---------- Строка 0: температура / уставка ----------
  if (isnan(sensor.temp()))
    snprintf(l0, sizeof(l0), "T=--.-  ERR:%s", sensor.statusStr());
  else if (wide)
    snprintf(l0, sizeof(l0), "T=%5.1fC  SP:%3.0fC", (double)sensor.temp(), (double)storage.s.setpoint);
  else
    snprintf(l0, sizeof(l0), "T=%5.1fC SP:%3.0f", (double)sensor.temp(), (double)storage.s.setpoint);

  // ---------- Строка 1: состояние / время / мощность ----------
  switch (machine.state()) {
    case CState::IDLE:
      snprintf(l1, sizeof(l1), wide ? "IDLE   hold %3dmin" : "IDLE hold %3dmin", storage.s.holdMin);
      break;
    case CState::HEATING:
      fmtTime(tt, sizeof(tt), machine.cycleSec());
      snprintf(l1, sizeof(l1), wide ? "HEAT   %s  P:%3.0f%%" : "HEAT %s P%3.0f",
               tt, (double)machine.lastPower());
      break;
    case CState::HOLD:
      fmtTime(tt, sizeof(tt), (uint32_t)machine.remainSec());
      snprintf(l1, sizeof(l1), wide ? "%s %s  P:%3.0f%%" : "%s %s P%3.0f",
               machine.holdPaused() ? "PAUSE:" : "HOLD: ", tt, (double)machine.lastPower());
      break;
    case CState::DONE:
      snprintf(l1, sizeof(l1), wide ? "DONE! hold finished" : "DONE! cycle ok");
      break;
    case CState::ALARM:
      snprintf(l1, sizeof(l1), wide ? "ALARM! see line 4" : "ALARM %.10s", machine.alarmMsg());
      break;
    default:
      snprintf(l1, sizeof(l1), "?");
  }

  _lcd.setCursor(0, 0); _lcd.print(l0);
  _lcd.setCursor(0, 1); _lcd.print(l1);

  // ---------- Строки 2–3: только для 20×4 ----------
  if (LCD_ROWS >= 4) {
    char l2[21], l3[21];

    if (webui.apMode())
      snprintf(l2, sizeof(l2), "W:AP %s", AP_SSID);
    else if (WiFi.status() == WL_CONNECTED)
      snprintf(l2, sizeof(l2), "W:OK %s", WiFi.localIP().toString().c_str());
    else
      snprintf(l2, sizeof(l2), "W:-- offline");

    if (machine.state() == CState::ALARM)
      snprintf(l3, sizeof(l3), "%.20s", machine.alarmMsg());
    else if (!sensor.ok())
      snprintf(l3, sizeof(l3), "SENSOR: %s", sensor.statusStr());
    else
      snprintf(l3, sizeof(l3), "cal:%+4.1fC  log:%3u", (double)storage.s.calOffset, (unsigned)machine.logLen);

    _lcd.setCursor(0, 2); _lcd.print(l2);
    _lcd.setCursor(0, 3); _lcd.print(l3);
  }
}

