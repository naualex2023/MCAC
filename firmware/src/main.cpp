// ============================================================
//  MCAC — блок управления автоклавом. Прошивка v0.1
//  ESP32 DevKit + LCD1602(I2C) + SSR-25DA + NTC
//  Архитектура: кооперативный loop (без RTOS-задач):
//   - 1 Гц   : sensor.tick() → machine.tick()  (регулирование, аварии)
//   - 2 Гц   : ui.update()                    (LCD)
//   - каждый : machine.service() (TPPWM на SSR, зуммер), webui.service()
// ============================================================
#include <Arduino.h>
#include "config.h"
#include "storage.h"
#include "sensor.h"
#include "pid.h"
#include "machine.h"
#include "ui_lcd.h"
#include "webui.h"

static uint32_t t1Hz = 0, t2Hz = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n[MCAC] firmware v%s\n", FW_VERSION);

  storage.begin();
  storage.load();

  machine.begin();   // SSR в безопасное состояние
  sensor.begin();
  pid.setGains(storage.s.kp, storage.s.ki, storage.s.kd);
  ui.begin();
  webui.begin();

  Serial.printf("[MCAC] init done. SP=%.1fC hold=%dmin cal=%+.1fC\n",
                storage.s.setpoint, storage.s.holdMin, storage.s.calOffset);
}

void loop() {
  const uint32_t now = millis();

  if (now - t1Hz >= 1000) {   // --- контур регулирования, 1 Гц ---
    t1Hz = now;
    sensor.tick();
    machine.tick();
  }

  if (now - t2Hz >= 500) {    // --- индикация, 2 Гц ---
    t2Hz = now;
    ui.update();
  }

  machine.service();          // TPPWM на SSR + зуммер (каждый проход)
  webui.service();            // mDNS, ArduinoOTA, отложенный рестарт
}
