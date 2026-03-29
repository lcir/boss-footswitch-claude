#pragma once
#include <Arduino.h>
#include <functional>
#include "config.h"

enum class Mode : uint8_t { NORMAL, EFFECTS };

// Event fired when a logical action should happen
struct FootswitchEvent {
    enum class Type : uint8_t {
        SELECT_PATCH,    // normal mode: select A1/A2/B1/B2
        TOGGLE_SOLO,     // normal mode: toggle solo
        TOGGLE_EFFECT,   // effects mode: toggle an effect
        MODE_CHANGED,    // mode was switched
    };
    Type    type;
    uint8_t index;   // patch index (0-3) or effect cc-index (CC_BOOST..CC_REVERB)
    bool    value;   // for toggles: new state
};

using EventCallback = std::function<void(const FootswitchEvent&)>;

class Footswitch {
public:
    void begin();
    void update();  // call from loop()

    Mode getMode()        const { return _mode; }
    bool isSoloActive()   const { return _soloActive; }
    bool isEffectActive(uint8_t idx) const { return _effectActive[idx]; }

    // Current selected patch index 0-3 (A1/A2/B1/B2)
    uint8_t getSelectedPatch() const { return _selectedPatch; }

    void onEvent(EventCallback cb) { _onEvent = cb; }

    // Trigger a button press programmatically (from web interface)
    void triggerButton(uint8_t swIdx);

private:
    static constexpr uint8_t DEBOUNCE_MS = 30;

    uint8_t  _pins[NUM_SWITCHES];
    bool     _lastState[NUM_SWITCHES];
    uint32_t _lastChangeMs[NUM_SWITCHES];

    Mode    _mode          = Mode::NORMAL;
    uint8_t _selectedPatch = 0;   // 0=A1 1=A2 2=B1 3=B2
    bool    _soloActive    = false;
    bool    _effectActive[6] = {};

    EventCallback _onEvent;

    void handlePress(uint8_t swIdx);
};

extern Footswitch Pedal;
