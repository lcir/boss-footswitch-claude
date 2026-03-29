#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <functional>

// BLE-MIDI service / characteristic UUIDs (standard BLE-MIDI 1.0)
static constexpr const char* BLE_MIDI_SERVICE_UUID = "03B80E5A-EDE8-4B33-A751-6CE34EC4C700";
static constexpr const char* BLE_MIDI_CHAR_UUID    = "7772E5DB-3868-4112-A1A9-F2669D106BF3";

using ConnectCallback    = std::function<void()>;
using DisconnectCallback = std::function<void()>;

class BleMidiClient {
public:
    void begin(const char* deviceName = "BossFootswitch");

    bool isConnected() const;
    bool isScanning()  const { return _scanning; }

    void update();  // call from loop()

    // MIDI send helpers (channel is 1-based, 1–16)
    void sendProgramChange(uint8_t channel, uint8_t program);
    void sendControlChange(uint8_t channel, uint8_t cc, uint8_t value);

    void onConnect(ConnectCallback cb)       { _onConnect = cb; }
    void onDisconnect(DisconnectCallback cb) { _onDisconnect = cb; }

    // Accessible by NimBLE callback classes defined in .cpp
    ConnectCallback    _onConnect;
    DisconnectCallback _onDisconnect;
    bool _connected         = false;
    bool _scanning          = false;
    bool _encrypted         = false;
    bool _connectPending    = false;   // set by scan CB, consumed by update()
    NimBLEAddress _pendingAddr = NimBLEAddress("");

    uint32_t _lastScanMs    = 0;

private:
    void startScan();
    void doConnect();
    void sendRaw(const uint8_t* data, size_t len);
};

extern BleMidiClient BleMidi;
