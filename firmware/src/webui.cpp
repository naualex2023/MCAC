#include "webui.h"
#include "config.h"
#include "storage.h"
#include "sensor.h"
#include "pid.h"
#include "machine.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

WebUi webui;

static AsyncWebServer server(80);

// ---------- Встроенная страница (PROGMEM) ----------
static const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MCAC автоклав</title><style>
body{font-family:system-ui;margin:14px;background:#12141a;color:#eee}
.card{background:#1e2128;border-radius:10px;padding:14px;margin-bottom:12px}
.big{font-size:2.4rem;font-weight:600}
label{display:inline-block;width:100px}input{width:100px;background:#2a2d33;color:#eee;
border:1px solid #444;border-radius:5px;padding:4px}
button{background:#3a7bd5;color:#fff;border:0;border-radius:6px;padding:9px 18px;
margin:4px;font-size:1rem}button.stop{background:#c0392b}
a{color:#7fb3ff}.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
</style></head><body><h2>Автоклав MCAC</h2>
<div class="card"><div class="row"><span>Состояние: <b id="st">—</b></span>
<span>Датчик: <b id="sn">—</b></span></div>
<div class="big"><span id="t">--.-</span> °C</div>
<div>Уставка: <b id="sp">—</b> °C · Мощность: <b id="out">—</b>%</div>
<div>Выдержка: <b id="rem">—</b> · Цикл: <b id="cyc">—</b> <span id="pa" style="color:#f0ad4e"></span></div>
<div id="al" style="color:#ff6b6b;font-weight:600"></div></div>
<div class="card"><button onclick="cmd('start')">Старт</button>
<button class="stop" onclick="cmd('stop')">Стоп</button></div>
<div class="card"><h3>Параметры</h3>
<div class="row"><label>Уставка °C</label><input id="i_sp" type="number" step="0.5">
<label>Выдержка мин</label><input id="i_hm" type="number" step="1"></div>
<div class="row"><label>Калибр. ±°C</label><input id="i_cal" type="number" step="0.1">
<label>Kp / Ki / Kd</label><input id="i_kp" type="number" step="0.5">
<input id="i_ki" type="number" step="0.05"><input id="i_kd" type="number" step="5"></div>
<button onclick="save()">Сохранить</button></div>
<div class="card"><h3>Wi-Fi</h3><div class="row">
<input id="w_ssid" placeholder="SSID" style="width:150px">
<input id="w_pass" placeholder="пароль" style="width:150px">
<button onclick="setwifi()">Подключить</button></div>
<div>IP: <b id="ip">—</b> · <a href="/api/log" target="_blank">Журнал CSV</a></div></div>
<script>
const $=i=>document.getElementById(i);
function fmt(s){if(s==null)return '—';s=Math.max(0,s);const m=Math.floor(s/60),x=s%60;
return String(m).padStart(2,'0')+':'+String(x).padStart(2,'0');}
async function load(){try{const r=await fetch('/api/status');const d=await r.json();
$('st').textContent=d.state;$('sn').textContent=d.sensorOk?'OK':d.sensorErr;
$('t').textContent=d.temp==null?'--.-':(+d.temp).toFixed(1);
$('sp').textContent=d.setpoint;$('out').textContent=d.out.toFixed(0);
$('rem').textContent=fmt(d.remain);$('cyc').textContent=fmt(d.cycle);
$('pa').textContent=d.paused?'(пауза: T ниже уставки)':'';
$('al').textContent=d.alarm||'';$('ip').textContent=d.ip;
if(!load.f){load.f=1;$('i_sp').value=d.setpoint;$('i_hm').value=d.holdMin;
$('i_cal').value=d.calOffset;$('i_kp').value=d.kp;$('i_ki').value=d.ki;$('i_kd').value=d.kd;}
}catch(e){$('st').textContent='нет связи';}}
async function post(u,b){await fetch(u,{method:'POST',
headers:{'Content-Type':'application/json'},body:JSON.stringify(b)});load();}
function cmd(a){post('/api/cmd',{action:a});}
function save(){post('/api/cmd',{action:'set',setpoint:+$('i_sp').value,holdMin:+$('i_hm').value,
calOffset:+$('i_cal').value,kp:+$('i_kp').value,ki:+$('i_ki').value,kd:+$('i_kd').value});}
async function setwifi(){await post('/api/wifi',{ssid:$('w_ssid').value,pass:$('w_pass').value});
alert('Сохранено. Плата перезагрузится и подключится к сети.');}
setInterval(load,2000);load();
</script></body></html>)html";

// ---------- JSON статуса ----------
static String statusJson() {
  JsonDocument doc;
  doc["state"]     = cstateStr(machine.state());
  doc["sensorOk"]  = sensor.ok();
  doc["sensorErr"] = sensor.statusStr();
  if (!isnan(sensor.temp())) doc["temp"] = sensor.temp(); // иначе поле null
  doc["setpoint"]  = storage.s.setpoint;
  doc["holdMin"]   = storage.s.holdMin;
  doc["remain"]    = (machine.state() == CState::HOLD) ? machine.remainSec() : -1;
  doc["cycle"]     = machine.cycleSec();
  doc["out"]       = machine.lastPower();
  doc["paused"]    = machine.holdPaused();
  doc["alarm"]     = machine.alarmMsg();
  doc["calOffset"] = storage.s.calOffset;
  doc["kp"] = storage.s.kp; doc["ki"] = storage.s.ki; doc["kd"] = storage.s.kd;
  doc["ip"]   = webui.apMode() ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["heap"] = ESP.getFreeHeap();
  String out; serializeJson(doc, out);
  return out;
}

// ---------- Журнал CSV ----------
static String logCsv() {
  String csv;
  csv.reserve(40 + machine.logLen * 30);
  csv += "t_s;temp_C;out_pct;state\n";
  for (size_t i = 0; i < machine.logLen; i++) {
    csv += machine.log[i].t; csv += ';';
    csv += String(machine.log[i].temp, 1); csv += ';';
    csv += String(machine.log[i].out);     csv += ';';   // uint8_t → число!
    csv += String(machine.log[i].state);   csv += '\n';  // uint8_t → число!
  }
  return csv;
}

// ---------- Обработчики команд ----------
static void handleCmd(uint8_t *data, size_t len) {
  JsonDocument doc;
  if (deserializeJson(doc, data, len)) return;

  const char *action = doc["action"] | "";
  if (strcmp(action, "start") == 0) machine.start();
  else if (strcmp(action, "stop") == 0) machine.stop();

  if (!doc["setpoint"].isNull()) {
    float v = doc["setpoint"];
    if (v >= SETPOINT_MIN && v <= SETPOINT_MAX) storage.s.setpoint = v;
  }
  if (!doc["holdMin"].isNull()) {
    int v = doc["holdMin"];
    if (v >= 1 && v <= 999) storage.s.holdMin = v;
  }
  if (!doc["calOffset"].isNull()) {
    float v = doc["calOffset"];
    if (v >= -10.0f && v <= 10.0f) storage.s.calOffset = v;
  }
  if (!doc["kp"].isNull()) storage.s.kp = doc["kp"].as<float>();
  if (!doc["ki"].isNull()) storage.s.ki = doc["ki"].as<float>();
  if (!doc["kd"].isNull()) storage.s.kd = doc["kd"].as<float>();
  pid.setGains(storage.s.kp, storage.s.ki, storage.s.kd);
  storage.save();
}

// ---------- Wi-Fi ----------
void WebUi::begin() {
  // STA из сохранённых настроек; 10 с на подключение
  if (strlen(storage.s.wifiSsid) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(storage.s.wifiSsid, storage.s.wifiPass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) { // фолбэк: открытая точка-конфигуратор
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    _apMode = true;
    Serial.printf("[WEB] AP mode: ssid=%s ip=%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.printf("[WEB] STA connected: %s ip=%s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }

  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "application/json", statusJson());
  });
  server.on("/api/cmd", HTTP_POST,
    [](AsyncWebServerRequest *r) { r->send(200, "application/json", "{\"ok\":true}"); },
    NULL,
    [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t i, size_t t) {
      handleCmd(data, len);
    });
  server.on("/api/wifi", HTTP_POST,
    [this](AsyncWebServerRequest *r) { r->send(200, "application/json", "{\"ok\":true}"); _restartAtMs = millis() + 800; },
    NULL,
    [this](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t i, size_t t) {
      JsonDocument doc;
      if (deserializeJson(doc, data, len)) return;
      strlcpy(storage.s.wifiSsid, doc["ssid"] | "", sizeof(storage.s.wifiSsid));
      strlcpy(storage.s.wifiPass, doc["pass"] | "", sizeof(storage.s.wifiPass));
      storage.save();
    });
  server.on("/api/log", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send(200, "text/csv", logCsv());
  });
  server.onNotFound([](AsyncWebServerRequest *r) { r->send(404, "text/plain", "404"); });
  server.begin();

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
}

void WebUi::service() {
  if (_restartAtMs && millis() > _restartAtMs) ESP.restart();
  MDNS.update();
  ArduinoOTA.handle();
}

