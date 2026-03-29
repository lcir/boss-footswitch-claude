#include "config.h"
#include <Preferences.h>

AppConfig Config::cfg;

static Preferences prefs;

static void setDefaults() {
    memset(&Config::cfg, 0, sizeof(Config::cfg));
    Config::cfg.midi.channel = DEFAULT_MIDI_CHANNEL;
    Config::cfg.midi.pc[0]   = DEFAULT_PC_A1;
    Config::cfg.midi.pc[1]   = DEFAULT_PC_A2;
    Config::cfg.midi.pc[2]   = DEFAULT_PC_B1;
    Config::cfg.midi.pc[3]   = DEFAULT_PC_B2;
    Config::cfg.midi.cc[CC_SOLO]   = DEFAULT_CC_SOLO;
    Config::cfg.midi.cc[CC_BOOST]  = DEFAULT_CC_BOOST;
    Config::cfg.midi.cc[CC_MOD]    = DEFAULT_CC_MOD;
    Config::cfg.midi.cc[CC_DELAY]  = DEFAULT_CC_DELAY;
    Config::cfg.midi.cc[CC_FX]     = DEFAULT_CC_FX;
    Config::cfg.midi.cc[CC_REVERB] = DEFAULT_CC_REVERB;
    strncpy(Config::cfg.bleTargetName, DEFAULT_BLE_TARGET_NAME,
            sizeof(Config::cfg.bleTargetName) - 1);
}

void Config::load() {
    setDefaults();
    prefs.begin("bossfs", true);  // read-only
    prefs.getBytes("cfg", &cfg, sizeof(cfg));
    prefs.end();
    // Ensure null-termination of strings in case of partial read
    cfg.wifiSsid[sizeof(cfg.wifiSsid) - 1] = '\0';
    cfg.wifiPass[sizeof(cfg.wifiPass) - 1] = '\0';
    cfg.bleTargetName[sizeof(cfg.bleTargetName) - 1] = '\0';
    // If bleTargetName is empty after load (e.g. first boot), set default
    if (cfg.bleTargetName[0] == '\0') {
        strncpy(cfg.bleTargetName, DEFAULT_BLE_TARGET_NAME,
                sizeof(cfg.bleTargetName) - 1);
    }
}

void Config::save() {
    prefs.begin("bossfs", false);
    prefs.putBytes("cfg", &cfg, sizeof(cfg));
    prefs.end();
}

void Config::saveWifi(const char* ssid, const char* pass) {
    strncpy(cfg.wifiSsid, ssid, sizeof(cfg.wifiSsid) - 1);
    strncpy(cfg.wifiPass, pass, sizeof(cfg.wifiPass) - 1);
    save();
}

bool Config::hasWifi() {
    return cfg.wifiSsid[0] != '\0';
}

void Config::toJson(JsonDocument& doc) {
    doc["ble_target"] = cfg.bleTargetName;
    doc["midi_channel"] = cfg.midi.channel;
    JsonObject pc = doc["pc"].to<JsonObject>();
    pc["A1"] = cfg.midi.pc[0];
    pc["A2"] = cfg.midi.pc[1];
    pc["B1"] = cfg.midi.pc[2];
    pc["B2"] = cfg.midi.pc[3];
    JsonObject cc = doc["cc"].to<JsonObject>();
    cc["solo"]   = cfg.midi.cc[CC_SOLO];
    cc["boost"]  = cfg.midi.cc[CC_BOOST];
    cc["mod"]    = cfg.midi.cc[CC_MOD];
    cc["delay"]  = cfg.midi.cc[CC_DELAY];
    cc["fx"]     = cfg.midi.cc[CC_FX];
    cc["reverb"] = cfg.midi.cc[CC_REVERB];
}

void Config::fromJson(const JsonDocument& doc) {
    if (doc["ble_target"].is<const char*>()) {
        strncpy(cfg.bleTargetName, doc["ble_target"].as<const char*>(),
                sizeof(cfg.bleTargetName) - 1);
    }
    if (doc["midi_channel"].is<uint8_t>())
        cfg.midi.channel = doc["midi_channel"].as<uint8_t>();
    if (doc["pc"].is<JsonObjectConst>()) {
        auto pc = doc["pc"].as<JsonObjectConst>();
        if (pc["A1"].is<uint8_t>()) cfg.midi.pc[0] = pc["A1"].as<uint8_t>();
        if (pc["A2"].is<uint8_t>()) cfg.midi.pc[1] = pc["A2"].as<uint8_t>();
        if (pc["B1"].is<uint8_t>()) cfg.midi.pc[2] = pc["B1"].as<uint8_t>();
        if (pc["B2"].is<uint8_t>()) cfg.midi.pc[3] = pc["B2"].as<uint8_t>();
    }
    if (doc["cc"].is<JsonObjectConst>()) {
        auto cc = doc["cc"].as<JsonObjectConst>();
        if (cc["solo"].is<uint8_t>())   cfg.midi.cc[CC_SOLO]   = cc["solo"].as<uint8_t>();
        if (cc["boost"].is<uint8_t>())  cfg.midi.cc[CC_BOOST]  = cc["boost"].as<uint8_t>();
        if (cc["mod"].is<uint8_t>())    cfg.midi.cc[CC_MOD]    = cc["mod"].as<uint8_t>();
        if (cc["delay"].is<uint8_t>())  cfg.midi.cc[CC_DELAY]  = cc["delay"].as<uint8_t>();
        if (cc["fx"].is<uint8_t>())     cfg.midi.cc[CC_FX]     = cc["fx"].as<uint8_t>();
        if (cc["reverb"].is<uint8_t>()) cfg.midi.cc[CC_REVERB] = cc["reverb"].as<uint8_t>();
    }
    save();
}
