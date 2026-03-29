#include <Arduino.h>
#include "config.h"
#include "ble_midi_client.h"
#include "footswitch.h"
#include "led_controller.h"
#include "wifi_manager.h"
#include "web_interface.h"

// ── Forward declarations ──────────────────────────────────────────────────────
static void onFootswitchEvent(const FootswitchEvent& evt);
static void onBleConnect();
static void onBleDisconnect();
static void onWifiConnected(const char* ip);

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\n[Main] Boss Footswitch booting…");

    Config::load();

    Leds.begin();
    Leds.flash(CRGB(0, 0, 80), 2, 100);  // boot indicator: blue flashes

    Pedal.begin();
    Pedal.onEvent(onFootswitchEvent);

    BleMidi.begin("BossFootswitch");
    BleMidi.onConnect(onBleConnect);
    BleMidi.onDisconnect(onBleDisconnect);

    WifiMgr.begin();
    WifiMgr.onConnected(onWifiConnected);

    // Web interface is started once WiFi connects (or immediately in AP mode)
    WebUI.begin();

    Serial.println("[Main] Boot complete");
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    Pedal.update();
    BleMidi.update();
    WifiMgr.update();
    Leds.update();
}

// ── Event handlers ────────────────────────────────────────────────────────────
static void onFootswitchEvent(const FootswitchEvent& evt) {
    const MidiConfig& mc = Config::cfg.midi;

    switch (evt.type) {
        case FootswitchEvent::Type::SELECT_PATCH:
            BleMidi.sendProgramChange(mc.channel, mc.pc[evt.index]);
            break;

        case FootswitchEvent::Type::TOGGLE_SOLO:
            BleMidi.sendControlChange(mc.channel,
                                      mc.cc[CC_SOLO],
                                      evt.value ? 127 : 0);
            break;

        case FootswitchEvent::Type::TOGGLE_EFFECT:
            BleMidi.sendControlChange(mc.channel,
                                      mc.cc[evt.index],
                                      evt.value ? 127 : 0);
            break;

        case FootswitchEvent::Type::MODE_CHANGED:
            Leds.flash(evt.value ? CRGB(200, 100, 0) : CRGB(0, 200, 0), 1, 60);
            break;
    }

    // Refresh LEDs and push state to web clients
    Leds.refresh(Pedal.getMode(), Pedal.getSelectedPatch(),
                 Pedal.isSoloActive(),
                 // Pass pointer to internal effect state array via helper
                 [&]() -> const bool* {
                     static bool tmp[6];
                     for (uint8_t i = 0; i < 6; i++) tmp[i] = Pedal.isEffectActive(i);
                     return tmp;
                 }(),
                 BleMidi.isConnected());

    WebUI.broadcastState();
}

static void onBleConnect() {
    Serial.println("[Main] BLE connected → refresh LEDs");
    Leds.flash(CRGB(0, 200, 0), 2, 80);
    // Trigger a LED refresh with current state
    bool effects[6];
    for (uint8_t i = 0; i < 6; i++) effects[i] = Pedal.isEffectActive(i);
    Leds.refresh(Pedal.getMode(), Pedal.getSelectedPatch(),
                 Pedal.isSoloActive(), effects, true);
    WebUI.broadcastState();
}

static void onBleDisconnect() {
    Serial.println("[Main] BLE disconnected → blink LEDs");
    // led_controller::update() will handle the blink
    bool effects[6] = {};
    Leds.refresh(Pedal.getMode(), Pedal.getSelectedPatch(),
                 Pedal.isSoloActive(), effects, false);
    WebUI.broadcastState();
}

static void onWifiConnected(const char* ip) {
    Serial.printf("[Main] WiFi connected — %s\n", ip);
    Leds.flash(CRGB(0, 100, 200), 3, 60);
}
