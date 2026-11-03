#pragma once
#include <Arduino.h>

// Wi-Fi STA (с фолбэком в AP-конфигуратор) + веб-интерфейс + ArduinoOTA
class WebUi {
public:
  void begin();    // вызывать один раз в setup()
  void service();  // вызывать каждый проход loop()
  bool apMode() const { return _apMode; }
private:
  bool     _apMode = false;
  uint32_t _restartAtMs = 0;
};

extern WebUi webui;
