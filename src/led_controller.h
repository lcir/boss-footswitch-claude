#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"
#include "footswitch.h"

class LedController {
public:
    void begin();
    void update();  // call from loop()

    // Refresh LEDs based on current pedal state
    void refresh(Mode mode, uint8_t selectedPatch, bool soloActive,
                 const bool* effectActive, bool bleConnected);

    // Show a brief blink animation on all LEDs (e.g. on mode change)
    void flash(CRGB color, uint8_t times = 2, uint16_t ms = 80);

private:
    CRGB _leds[NUM_LEDS];

    bool     _bleConnected  = true;
    uint32_t _animTimer     = 0;

    // Knight-rider sweep state
    int8_t   _sweepPos      = 0;
    int8_t   _sweepDir      = 1;
    static constexpr uint16_t SWEEP_STEP_MS  = 80;   // speed of sweep

    // Waiting-to-reconnect pulse state
    uint8_t  _pulseVal      = 0;
    int8_t   _pulseDir      = 4;
    static constexpr uint16_t PULSE_STEP_MS  = 30;   // speed of pulse
};

extern LedController Leds;
