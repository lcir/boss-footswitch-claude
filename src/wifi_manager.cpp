#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

WiFiManager WifiMgr;

static DNSServer dnsServer;
static bool      dnsRunning = false;

static constexpr const char* AP_SSID     = "BossFootswitch-Setup";
static constexpr const char* HOSTNAME    = "boss-footswitch";
static constexpr uint8_t     DNS_PORT    = 53;

// ── Public ────────────────────────────────────────────────────────────────────
void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);

    if (Config::hasWifi()) {
        Serial.printf("[WiFi] Connecting to SSID: %s\n", Config::cfg.wifiSsid);
        startSTA(Config::cfg.wifiSsid, Config::cfg.wifiPass);
    } else {
        Serial.println("[WiFi] No credentials — starting AP");
        startAP();
    }
}

void WiFiManager::update() {
    if (dnsRunning) {
        dnsServer.processNextRequest();
    }

    if (_state == WiFiState::CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WiFiState::CONNECTED;
            stopDNSServer();
            WiFi.mode(WIFI_STA);
            startMDNS();
            char ip[16];
            WiFi.localIP().toString().toCharArray(ip, sizeof(ip));
            Serial.printf("[WiFi] Connected — IP: %s\n", ip);
            if (_onConnected) _onConnected(ip);
        } else if (millis() - _connectStart > CONNECT_TIMEOUT_MS) {
            Serial.println("[WiFi] Connection timeout — falling back to AP");
            WiFi.disconnect(true);
            startAP();
        }
    }
}

void WiFiManager::saveAndConnect(const char* ssid, const char* pass) {
    Config::saveWifi(ssid, pass);
    Serial.printf("[WiFi] Saved credentials, connecting to %s\n", ssid);
    stopDNSServer();
    startSTA(ssid, pass);
}

// ── Private ───────────────────────────────────────────────────────────────────
void WiFiManager::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    delay(100);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WiFi] AP started — SSID: %s  IP: %s\n", AP_SSID, apIP.toString().c_str());

    startDNSServer();
    _state = WiFiState::AP_MODE;
}

void WiFiManager::startSTA(const char* ssid, const char* pass) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    _connectStart = millis();
    _state        = WiFiState::CONNECTING;
}

void WiFiManager::startMDNS() {
    if (MDNS.begin(HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[mDNS] Hostname: %s.local\n", HOSTNAME);
    }
}

void WiFiManager::startDNSServer() {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsRunning = true;
}

void WiFiManager::stopDNSServer() {
    if (dnsRunning) {
        dnsServer.stop();
        dnsRunning = false;
    }
}
