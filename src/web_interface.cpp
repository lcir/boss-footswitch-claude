#include "web_interface.h"
#include "config.h"
#include "footswitch.h"
#include "ble_midi_client.h"
#include "wifi_manager.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <stdarg.h>

WebInterface WebUI;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// ── WebSocket event handler ───────────────────────────────────────────────────
static void onWsEvent(AsyncWebSocket* srv, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u connected\n", client->id());
        WebUI.broadcastState();
        // Send full log history to the newly connected client
        uint8_t  count = min((uint8_t)WebUI._logCount, (uint8_t)WebInterface::LOG_BUF_SIZE);
        uint8_t  start = (WebUI._logCount >= WebInterface::LOG_BUF_SIZE)
                         ? WebUI._logHead : 0;
        for (uint8_t i = 0; i < count; i++) {
            const LogEntry& e = WebUI._logBuf[(start + i) % WebInterface::LOG_BUF_SIZE];
            JsonDocument doc;
            doc["type"]  = "log";
            doc["ms"]    = e.ms;
            doc["level"] = (uint8_t)e.level;
            doc["msg"]   = e.msg;
            String json; serializeJson(doc, json);
            client->text(json);
        }
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            char msg[256];
            size_t copyLen = min(len, sizeof(msg) - 1);
            memcpy(msg, data, copyLen);
            msg[copyLen] = '\0';
            WebUI.handleWsMessage(msg, copyLen);
        }
    }
}

// ── Captive portal HTML (inline, served when LittleFS not mounted) ────────────
static const char CAPTIVE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Boss Footswitch Setup</title>
<style>
  body{font-family:sans-serif;background:#1a1a1a;color:#eee;display:flex;
       justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#2a2a2a;padding:2rem;border-radius:12px;width:320px}
  h1{margin:0 0 1.5rem;font-size:1.4rem;text-align:center}
  label{display:block;margin-bottom:.3rem;font-size:.9rem;color:#aaa}
  input{width:100%;box-sizing:border-box;padding:.6rem;border-radius:6px;
        border:1px solid #444;background:#111;color:#eee;margin-bottom:1rem;font-size:1rem}
  button{width:100%;padding:.7rem;border:none;border-radius:6px;
         background:#0a7;color:#fff;font-size:1rem;cursor:pointer}
  button:hover{background:#0c9}
  .msg{text-align:center;margin-top:1rem;color:#6f6}
</style></head><body>
<div class="card">
  <h1>Boss Footswitch<br>WiFi Setup</h1>
  <form id="f">
    <label>Network (SSID)</label>
    <input type="text" id="s" placeholder="My WiFi" required>
    <label>Password</label>
    <input type="password" id="p" placeholder="password">
    <button type="submit">Connect</button>
  </form>
  <div class="msg" id="msg"></div>
</div>
<script>
document.getElementById('f').onsubmit=function(e){
  e.preventDefault();
  fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({ssid:document.getElementById('s').value,
                         pass:document.getElementById('p').value})})
  .then(()=>{document.getElementById('msg').textContent='Connecting… device will restart.'})
  .catch(()=>{document.getElementById('msg').textContent='Error, try again.'});
};
</script></body></html>
)rawliteral";

// ── begin ─────────────────────────────────────────────────────────────────────
void WebInterface::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Web] LittleFS mount failed — serving captive portal only");
    }

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    setupRoutes();
    server.begin();
    Serial.println("[Web] HTTP server started");
}

// ── Routes ────────────────────────────────────────────────────────────────────
void WebInterface::setupRoutes() {
    // Captive portal WiFi setup endpoint (AP mode)
    server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
                const char* ssid = doc["ssid"] | "";
                const char* pass = doc["pass"] | "";
                if (strlen(ssid) > 0) {
                    req->send(200, "text/plain", "OK");
                    // Small delay so the response is sent before restarting
                    delay(500);
                    Config::saveWifi(ssid, pass);
                    ESP.restart();
                    return;
                }
            }
            req->send(400, "text/plain", "Bad Request");
        });

    // WiFi reset — clears saved credentials and reboots into AP mode
    server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(200, "text/plain", "OK");
        delay(300);
        Config::saveWifi("", "");
        ESP.restart();
    });

    // MIDI config GET/POST
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        Config::toJson(doc);
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* req) {}, nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
                Config::fromJson(doc);
                req->send(200, "text/plain", "OK");
            } else {
                req->send(400, "text/plain", "Bad JSON");
            }
        });

    // Root: captive portal in AP mode, main app in STA mode
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (WifiMgr.getState() == WiFiState::AP_MODE) {
            req->send(200, "text/html", CAPTIVE_HTML);
        } else {
            req->send(LittleFS, "/index.html", "text/html");
        }
    });

    // Static files (CSS, JS) — served from LittleFS regardless of mode
    server.serveStatic("/style.css", LittleFS, "/style.css");
    server.serveStatic("/app.js",    LittleFS, "/app.js");

    // Catch-all: redirect everything to root (triggers captive portal on mobile)
    server.onNotFound([](AsyncWebServerRequest* req) {
        req->redirect("/");
    });
}

// ── Broadcast state to all WS clients ────────────────────────────────────────
void WebInterface::broadcastState() {
    if (ws.count() == 0) return;

    JsonDocument doc;
    doc["type"]   = "state";
    doc["mode"]   = (Pedal.getMode() == Mode::NORMAL) ? "normal" : "effects";
    doc["patch"]  = Pedal.getSelectedPatch();
    doc["solo"]   = Pedal.isSoloActive();
    doc["ble"]    = BleMidi.isConnected();

    JsonObject fx = doc["effects"].to<JsonObject>();
    fx["boost"]  = Pedal.isEffectActive(CC_BOOST);
    fx["mod"]    = Pedal.isEffectActive(CC_MOD);
    fx["delay"]  = Pedal.isEffectActive(CC_DELAY);
    fx["fx"]     = Pedal.isEffectActive(CC_FX);
    fx["reverb"] = Pedal.isEffectActive(CC_REVERB);

    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

// ── Log ───────────────────────────────────────────────────────────────────────
void WebInterface::log(LogLevel level, const char* fmt, ...) {
    LogEntry& e = _logBuf[_logHead];
    e.ms    = millis();
    e.level = level;

    va_list args;
    va_start(args, fmt);
    vsnprintf(e.msg, sizeof(e.msg), fmt, args);
    va_end(args);

    Serial.printf("[LOG] %s\n", e.msg);

    _logHead = (_logHead + 1) % LOG_BUF_SIZE;
    if (_logCount < LOG_BUF_SIZE) _logCount++;

    broadcastLog(e);
}

void WebInterface::broadcastLog(const LogEntry& entry) {
    if (ws.count() == 0) return;
    JsonDocument doc;
    doc["type"]  = "log";
    doc["ms"]    = entry.ms;
    doc["level"] = (uint8_t)entry.level;
    doc["msg"]   = entry.msg;
    String json;
    serializeJson(doc, json);
    ws.textAll(json);
}

// ── Handle incoming WS message ────────────────────────────────────────────────
void WebInterface::handleWsMessage(const char* msg, size_t len) {
    JsonDocument doc;
    if (deserializeJson(doc, msg, len) != DeserializationError::Ok) return;

    const char* type = doc["type"] | "";

    if (strcmp(type, "press") == 0) {
        const char* btn = doc["button"] | "";
        uint8_t swIdx = 255;
        if      (strcmp(btn, "A1")   == 0) swIdx = SW_A1;
        else if (strcmp(btn, "A2")   == 0) swIdx = SW_A2;
        else if (strcmp(btn, "B1")   == 0) swIdx = SW_B1;
        else if (strcmp(btn, "B2")   == 0) swIdx = SW_B2;
        else if (strcmp(btn, "SOLO") == 0) swIdx = SW_SOLO;
        else if (strcmp(btn, "MODE") == 0) swIdx = SW_MODE;

        if (swIdx != 255) {
            Pedal.triggerButton(swIdx);
            broadcastState();
        }
    }
}
