#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// ── GPIO assignments ──────────────────────────────────────────────────────────
static constexpr uint8_t PIN_SW_A1   = 4;
static constexpr uint8_t PIN_SW_A2   = 5;
static constexpr uint8_t PIN_SW_B1   = 18;
static constexpr uint8_t PIN_SW_B2   = 19;
static constexpr uint8_t PIN_SW_SOLO = 21;
static constexpr uint8_t PIN_SW_MODE = 22;
static constexpr uint8_t PIN_LED_DATA = 23;

static constexpr uint8_t NUM_SWITCHES = 6;
static constexpr uint8_t NUM_LEDS     = 6;

// Switch indices (maps to above pins in order)
enum SwIdx : uint8_t { SW_A1 = 0, SW_A2, SW_B1, SW_B2, SW_SOLO, SW_MODE };

// ── MIDI defaults ─────────────────────────────────────────────────────────────
static constexpr uint8_t DEFAULT_MIDI_CHANNEL = 1;   // 1-based

// Program Change values for patch select (from Boss Tone Studio RX PC settings)
static constexpr uint8_t DEFAULT_PC_A1 = 1;   // RX PC A: CH-1
static constexpr uint8_t DEFAULT_PC_A2 = 2;   // RX PC A: CH-2
static constexpr uint8_t DEFAULT_PC_B1 = 6;   // RX PC B: CH-1
static constexpr uint8_t DEFAULT_PC_B2 = 7;   // RX PC B: CH-2

// CC numbers for effects (from Boss Tone Studio RX CC settings)
static constexpr uint8_t DEFAULT_CC_SOLO   = 73;  // (not shown in BTS, keep default)
static constexpr uint8_t DEFAULT_CC_BOOST  = 16;  // RX CC BOOSTER SW
static constexpr uint8_t DEFAULT_CC_MOD    = 17;  // RX CC MOD SW
static constexpr uint8_t DEFAULT_CC_DELAY  = 19;  // RX CC DELAY SW
static constexpr uint8_t DEFAULT_CC_FX     = 18;  // RX CC FX SW
static constexpr uint8_t DEFAULT_CC_REVERB = 20;  // RX CC REVERB SW

// ── Config struct ─────────────────────────────────────────────────────────────
struct MidiConfig {
    uint8_t channel;   // 1-based
    uint8_t pc[4];     // A1, A2, B1, B2
    uint8_t cc[6];     // solo, boost, mod, delay, fx, reverb
};

enum CcIdx : uint8_t { CC_SOLO = 0, CC_BOOST, CC_MOD, CC_DELAY, CC_FX, CC_REVERB };

// Default BLE device name to scan for (Katana 50 Gen3 with BT-Dual)
static constexpr const char* DEFAULT_BLE_TARGET_NAME = "KATANA 3 MIDI";

struct AppConfig {
    char     wifiSsid[64];
    char     wifiPass[64];
    char     bleTargetName[32];
    MidiConfig midi;
};

// ── Config manager ────────────────────────────────────────────────────────────
class Config {
public:
    static void     load();
    static void     save();
    static void     saveWifi(const char* ssid, const char* pass);
    static bool     hasWifi();
    static void     toJson(JsonDocument& doc);
    static void     fromJson(const JsonDocument& doc);

    static AppConfig cfg;
};
