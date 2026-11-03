#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

// LCD 1602 по I2C — только индикация (управление из веб-UI)
class UiLcd {
public:
  void begin();
  void update(); // вызывать каждые 500 мс
private:
  LiquidCrystal_I2C _lcd{LCD_ADDR, LCD_COLS, LCD_ROWS};
  bool _ok = false;
};

extern UiLcd ui;
