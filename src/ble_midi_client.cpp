#include "ble_midi_client.h"
#include "web_interface.h"
#include "config.h"
#include <NimBLEDevice.h>

BleMidiClient BleMidi;

// ── NimBLE handles ────────────────────────────────────────────────────────────
static NimBLEClient*               pClient   = nullptr;
static NimBLERemoteCharacteristic* pMidiChar = nullptr;

// ── Macro: log to both WebUI and Serial ───────────────────────────────────────
#define BLOG(level, fmt, ...) \
    do { \
        WebUI.log(level, fmt, ##__VA_ARGS__); \
        Serial.printf("[BLE] " fmt "\n", ##__VA_ARGS__); \
    } while(0)

// ── Security / client callback ────────────────────────────────────────────────
class SecurityCB : public NimBLEClientCallbacks {
public:
    void onConnect(NimBLEClient* client) override {
        BLOG(LogLevel::INFO, "Link navázán — iniciuji šifrování...");
        BleMidi._connected = true;
    }

    void onDisconnect(NimBLEClient* client) override {
        BLOG(LogLevel::WARN, "BT-Dual odpojen — hledám znovu...");
        BleMidi._connected   = false;
        BleMidi._encrypted   = false;
        pMidiChar = nullptr;
        if (BleMidi._onDisconnect) BleMidi._onDisconnect();
    }

    void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
        Serial.printf("[BLE] onAuthenticationComplete: encrypted=%d bonded=%d\n",
                      desc->sec_state.encrypted, desc->sec_state.bonded);
        if (desc->sec_state.encrypted) {
            BLOG(LogLevel::OK, "Šifrování OK (bonded=%d)", desc->sec_state.bonded);
            BleMidi._encrypted = true;
        } else {
            BLOG(LogLevel::ERR, "Šifrování selhalo — BT-Dual odmítl pairing");
            BleMidi._encrypted = false;
        }
    }

    bool onConfirmPIN(uint32_t pin) override {
        Serial.printf("[BLE] onConfirmPIN: %u — auto-confirming (Just Works)\n", pin);
        return true;
    }
} securityCB;

// ── Scan callback — only saves the address, does NOT connect ──────────────────
// Connecting from within the NimBLE scan callback would block the NimBLE host
// task waiting for connection events it can never deliver → timeout (err 0).
// Instead we set _connectPending so update() can connect from the Arduino loop.
class ScanCB : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice* device) override {
        std::string nameStr = device->getName();
        bool hasName = nameStr.length() > 0;
        const char* name = hasName ? nameStr.c_str() : "(bez názvu)";

        bool isTarget = hasName &&
            (strcmp(nameStr.c_str(), Config::cfg.bleTargetName) == 0);
        bool isMidi = device->isAdvertisingService(NimBLEUUID(BLE_MIDI_SERVICE_UUID));

        if (isTarget) {
            BLOG(LogLevel::OK, "★ CÍLE: \"%s\"  %s  RSSI %d dBm",
                 name, device->getAddress().toString().c_str(), device->getRSSI());
        } else if (isMidi) {
            BLOG(LogLevel::OK, "★ BLE-MIDI: \"%s\"  %s  RSSI %d dBm",
                 name, device->getAddress().toString().c_str(), device->getRSSI());
        } else {
            if (!hasName) return;
            BLOG(LogLevel::INFO, "  BLE: \"%s\"  %s  RSSI %d dBm",
                 name, device->getAddress().toString().c_str(), device->getRSSI());
            return;
        }

        // Save address and schedule connect from update() — do NOT connect here
        BleMidi._pendingAddr    = device->getAddress();
        BleMidi._connectPending = true;
        NimBLEDevice::getScan()->stop();
        Serial.println("[BLE] Scan stopped — connection scheduled from main loop");
    }
} scanCB;

// ── Public API ────────────────────────────────────────────────────────────────
void BleMidiClient::begin(const char* deviceName) {
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    // Just Works + bonding + Secure Connections (same as Codex project)
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

    Serial.printf("[BLE] init — hledám \"%s\"\n", Config::cfg.bleTargetName);
    BLOG(LogLevel::INFO, "BLE init (Just Works + bonding) — hledám \"%s\"",
         Config::cfg.bleTargetName);
    startScan();
}

bool BleMidiClient::isConnected() const {
    return _connected && _encrypted && pMidiChar != nullptr;
}

void BleMidiClient::update() {
    // ── Trigger pending connect from main loop (NOT from scan CB) ────────────
    if (_connectPending && !_connected) {
        _connectPending = false;
        doConnect();
        return;
    }

    if (_connected && _encrypted && pMidiChar != nullptr) return;

    // Silently-disconnected check
    if (pClient != nullptr && !pClient->isConnected()) {
        if (_connected) {
            Serial.println("[BLE] Detekováno tiché odpojení");
            _connected  = false;
            _encrypted  = false;
            pMidiChar   = nullptr;
            if (_onDisconnect) _onDisconnect();
        }
    }

    // Detect when scan has naturally completed (timed out)
    if (_scanning && !NimBLEDevice::getScan()->isScanning()) {
        _scanning = false;
        Serial.println("[BLE] Sken dokončen (timeout)");
    }

    // Re-scan every 5s if not connected and nothing pending
    if (!_scanning && !_connectPending && (millis() - _lastScanMs > 5000)) {
        startScan();
    }
}

void BleMidiClient::startScan() {
    _scanning   = true;
    _lastScanMs = millis();
    Serial.printf("[BLE] Spouštím sken — hledám \"%s\"...\n", Config::cfg.bleTargetName);
    BLOG(LogLevel::INFO, "Skenuji BLE — hledám \"%s\"...", Config::cfg.bleTargetName);

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(&scanCB, false);
    pScan->setActiveScan(true);
    pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL);
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->start(5, nullptr, false);  // 5s, non-blocking; scan-complete handled in update()
}

// ── doConnect — called from update() in Arduino main loop ────────────────────
void BleMidiClient::doConnect() {
    _scanning = false;
    NimBLEAddress addr = _pendingAddr;

    Serial.printf("[BLE] doConnect() → %s\n", addr.toString().c_str());
    BLOG(LogLevel::INFO, "Připojuji se k %s...", addr.toString().c_str());

    // ── Create / reuse client ────────────────────────────────────────────────
    if (pClient == nullptr) {
        Serial.println("[BLE] Vytvářím NimBLEClient");
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(&securityCB, false);
    } else if (pClient->isConnected()) {
        Serial.println("[BLE] Client stále připojen — odpojuji");
        pClient->disconnect();
        delay(500);
    }

    // Use default connection parameters (like Codex: NULL params)
    pClient->setConnectTimeout(30);

    Serial.println("[BLE] Volám pClient->connect()...");
    if (!pClient->connect(addr)) {
        int err = pClient->getLastError();
        Serial.printf("[BLE] connect() selhalo — err=%d\n", err);
        BLOG(LogLevel::ERR,
             "Připojení selhalo (err %d) — vytáhni/zastrč BT-Dual nebo vypni BT na telefonu",
             err);
        return;
    }

    Serial.println("[BLE] connect() OK — iniciuji secureConnection()");
    BLOG(LogLevel::OK, "Link navázán s %s", pClient->getPeerAddress().toString().c_str());

    // ── Security ─────────────────────────────────────────────────────────────
    BLOG(LogLevel::INFO, "Iniciuji BLE security (Just Works + bonding)...");
    bool secOk = pClient->secureConnection();
    Serial.printf("[BLE] secureConnection() = %s, _encrypted=%d\n",
                  secOk ? "true" : "false", (int)_encrypted);

    // Wait for encryption (onAuthenticationComplete sets _encrypted)
    uint32_t t0 = millis();
    while (!_encrypted && _connected && (millis() - t0 < 10000)) {
        delay(50);
    }
    Serial.printf("[BLE] Po čekání na šifrování: _encrypted=%d _connected=%d dt=%lums\n",
                  (int)_encrypted, (int)_connected, millis() - t0);

    if (!_encrypted) {
        BLOG(LogLevel::ERR,
             "Šifrování se nezdařilo do 10s — BT-Dual možná spárován s jiným zařízením");
        pClient->disconnect();
        return;
    }

    // ── MTU exchange (Codex does this before GATT discovery) ─────────────────
    Serial.printf("[BLE] MTU = %d\n", pClient->getMTU());

    // ── GATT: find MIDI service ───────────────────────────────────────────────
    Serial.println("[BLE] Hledám MIDI service...");
    BLOG(LogLevel::INFO, "Hledám BLE-MIDI service...");
    NimBLERemoteService* pSvc = pClient->getService(BLE_MIDI_SERVICE_UUID);
    if (!pSvc) {
        Serial.println("[BLE] MIDI service NENALEZEN");
        BLOG(LogLevel::ERR, "MIDI service nenalezen — zkontroluj BT-Dual firmware");
        pClient->disconnect();
        return;
    }
    Serial.println("[BLE] MIDI service nalezen");
    BLOG(LogLevel::OK, "MIDI service nalezen");

    // ── GATT: find MIDI characteristic ───────────────────────────────────────
    pMidiChar = pSvc->getCharacteristic(BLE_MIDI_CHAR_UUID);
    if (!pMidiChar) {
        Serial.println("[BLE] MIDI charakteristika NENALEZENA");
        BLOG(LogLevel::ERR, "MIDI charakteristika nenalezena");
        pClient->disconnect();
        return;
    }
    Serial.printf("[BLE] MIDI char nalezena: handle=%d canWrite=%d canWriteNR=%d canNotify=%d\n",
                  pMidiChar->getHandle(),
                  (int)pMidiChar->canWrite(),
                  (int)pMidiChar->canWriteNoResponse(),
                  (int)pMidiChar->canNotify());
    BLOG(LogLevel::OK, "MIDI char: handle %d canWrite=%d canNotify=%d",
         pMidiChar->getHandle(), pMidiChar->canWrite(), pMidiChar->canNotify());

    // ── Subscribe CCCD ────────────────────────────────────────────────────────
    if (pMidiChar->canNotify()) {
        Serial.println("[BLE] Subscribuji notifikace (CCCD)...");
        if (pMidiChar->subscribe(true, nullptr, true)) {
            Serial.println("[BLE] CCCD subscribe OK");
            BLOG(LogLevel::OK, "Notifikace aktivovány (CCCD zapsán)");
        } else {
            Serial.println("[BLE] CCCD subscribe SELHAL");
            BLOG(LogLevel::WARN, "CCCD subscribe selhal");
        }
    }

    // ── Ready ─────────────────────────────────────────────────────────────────
    Serial.println("[BLE] *** PŘIPOJENO A PŘIPRAVENO ***");
    BLOG(LogLevel::OK, "Připojeno! Zápis MIDI zpráv je k dispozici.");

    if (_onConnect) _onConnect();
}

// ── BLE-MIDI packet builder ───────────────────────────────────────────────────
static void buildPacket(uint8_t* buf, size_t& len,
                        uint8_t status, uint8_t d0, uint8_t d1, bool twoData) {
    uint32_t ms = millis() & 0x1FFF;
    buf[0] = 0x80 | ((ms >> 7) & 0x3F);
    buf[1] = 0x80 | (ms & 0x7F);
    buf[2] = status;
    buf[3] = d0 & 0x7F;
    if (twoData) { buf[4] = d1 & 0x7F; len = 5; }
    else         { len = 4; }
}

void BleMidiClient::sendRaw(const uint8_t* data, size_t len) {
    if (!isConnected()) return;
    if (pMidiChar->canWriteNoResponse()) {
        pMidiChar->writeValue(const_cast<uint8_t*>(data), len, false);
    } else if (pMidiChar->canWrite()) {
        pMidiChar->writeValue(const_cast<uint8_t*>(data), len, true);
    }
}

void BleMidiClient::sendProgramChange(uint8_t channel, uint8_t program) {
    if (!isConnected()) {
        BLOG(LogLevel::WARN, "MIDI PC ch %d prog %d — BLE nepřipojeno!", channel, program);
        return;
    }
    uint8_t buf[5]; size_t len;
    buildPacket(buf, len, 0xC0 | ((channel - 1) & 0x0F), program, 0, false);
    sendRaw(buf, len);
    BLOG(LogLevel::OK, "MIDI PC → ch %d  prog %d", channel, program);
}

void BleMidiClient::sendControlChange(uint8_t channel, uint8_t cc, uint8_t value) {
    if (!isConnected()) {
        BLOG(LogLevel::WARN, "MIDI CC ch %d cc %d — BLE nepřipojeno!", channel, cc);
        return;
    }
    uint8_t buf[5]; size_t len;
    buildPacket(buf, len, 0xB0 | ((channel - 1) & 0x0F), cc, value, true);
    sendRaw(buf, len);
    BLOG(LogLevel::OK, "MIDI CC → ch %d  cc %d  val %d", channel, cc, value);
}
