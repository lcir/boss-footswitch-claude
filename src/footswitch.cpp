#include "footswitch.h"
#include "config.h"

Footswitch Pedal;

static const uint8_t SWITCH_PINS[NUM_SWITCHES] = {
    PIN_SW_A1, PIN_SW_A2, PIN_SW_B1, PIN_SW_B2, PIN_SW_SOLO, PIN_SW_MODE
};

void Footswitch::begin() {
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        _pins[i]          = SWITCH_PINS[i];
        _lastState[i]     = HIGH;  // pulled up, not pressed
        _lastChangeMs[i]  = 0;
        pinMode(_pins[i], INPUT_PULLUP);
    }
}

void Footswitch::update() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        bool state = digitalRead(_pins[i]);
        if (state != _lastState[i]) {
            _lastChangeMs[i] = now;
            _lastState[i]    = state;
        }
        // Fire on falling edge (press down) after debounce
        if (!state && (now - _lastChangeMs[i] == DEBOUNCE_MS)) {
            handlePress(i);
        }
    }
}

void Footswitch::triggerButton(uint8_t swIdx) {
    if (swIdx < NUM_SWITCHES) handlePress(swIdx);
}

void Footswitch::handlePress(uint8_t swIdx) {
    FootswitchEvent evt{};

    if (swIdx == SW_MODE) {
        _mode = (_mode == Mode::NORMAL) ? Mode::EFFECTS : Mode::NORMAL;
        evt.type  = FootswitchEvent::Type::MODE_CHANGED;
        evt.value = (_mode == Mode::EFFECTS);
        Serial.printf("[SW] Mode → %s\n", _mode == Mode::EFFECTS ? "EFFECTS" : "NORMAL");
        if (_onEvent) _onEvent(evt);
        return;
    }

    if (_mode == Mode::NORMAL) {
        if (swIdx <= SW_B2) {
            // Patch select: A1=0, A2=1, B1=2, B2=3
            _selectedPatch = swIdx;
            evt.type  = FootswitchEvent::Type::SELECT_PATCH;
            evt.index = swIdx;
            Serial.printf("[SW] Patch %d selected\n", swIdx);
        } else if (swIdx == SW_SOLO) {
            _soloActive = !_soloActive;
            evt.type  = FootswitchEvent::Type::TOGGLE_SOLO;
            evt.value = _soloActive;
            Serial.printf("[SW] Solo → %s\n", _soloActive ? "ON" : "OFF");
        }
    } else {
        // EFFECTS mode — map buttons to effect CC indices
        // A1=BOOST, A2=MOD, B1=DELAY, B2=FX, SOLO=REVERB
        uint8_t ccIdx;
        switch (swIdx) {
            case SW_A1:   ccIdx = CC_BOOST;  break;
            case SW_A2:   ccIdx = CC_MOD;    break;
            case SW_B1:   ccIdx = CC_DELAY;  break;
            case SW_B2:   ccIdx = CC_FX;     break;
            case SW_SOLO: ccIdx = CC_REVERB; break;
            default: return;
        }
        _effectActive[ccIdx] = !_effectActive[ccIdx];
        evt.type  = FootswitchEvent::Type::TOGGLE_EFFECT;
        evt.index = ccIdx;
        evt.value = _effectActive[ccIdx];
        Serial.printf("[SW] Effect %d → %s\n", ccIdx, evt.value ? "ON" : "OFF");
    }

    if (_onEvent) _onEvent(evt);
}
