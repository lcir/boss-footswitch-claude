#pragma once
#include <Arduino.h>
#include <functional>

enum class WiFiState : uint8_t {
    IDLE,
    CONNECTING,
    CONNECTED,
    AP_MODE,       // captive-portal setup mode
};

using WiFiConnectedCallback = std::function<void(const char* ip)>;

class WiFiManager {
public:
    void begin();
    void update();  // call from loop()

    WiFiState getState() const { return _state; }
    bool      isConnected() const { return _state == WiFiState::CONNECTED; }

    void onConnected(WiFiConnectedCallback cb) { _onConnected = cb; }

    // Called by captive-portal handler after form submit
    void saveAndConnect(const char* ssid, const char* pass);

private:
    WiFiState _state     = WiFiState::IDLE;
    uint32_t  _connectStart = 0;
    static constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;

    WiFiConnectedCallback _onConnected;

    void startAP();
    void startSTA(const char* ssid, const char* pass);
    void startMDNS();
    void startDNSServer();
    void stopDNSServer();
};

extern WiFiManager WifiMgr;
