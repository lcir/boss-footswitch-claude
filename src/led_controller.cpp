#include "led_controller.h"
#include "ble_midi_client.h"

LedController Leds;

// Color palette (file-scope to avoid static constexpr ODR issues with CRGB struct)
static constexpr CRGB COL_INACTIVE     = CRGB(20, 20, 20);    // dim white
static constexpr CRGB COL_ACTIVE_PATCH = CRGB(0, 200, 0);     // green
static constexpr CRGB COL_SOLO_ON      = CRGB(200, 0, 0);     // red
static constexpr CRGB COL_FX_OFF       = CRGB(0, 0, 60);      // dim blue
static constexpr CRGB COL_FX_ON        = CRGB(0, 200, 200);   // cyan
static constexpr CRGB COL_MODE_EFFECTS = CRGB(200, 100, 0);   // orange
static constexpr CRGB COL_OFF          = CRGB(0, 0, 0);

// BLE animation colours
static constexpr CRGB COL_SWEEP_HEAD   = CRGB(0, 80, 255);    // bright blue — sweep head
static constexpr CRGB COL_SWEEP_MID    = CRGB(0, 20, 80);     // medium blue — trail
static constexpr CRGB COL_SWEEP_TAIL   = CRGB(0, 5, 20);      // dim blue — tail
static constexpr CRGB COL_WAIT_BASE    = CRGB(60, 30, 0);     // amber — waiting pulse base

void LedController::begin() {
    FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(_leds, NUM_LEDS);
    FastLED.setBrightness(120);
    fill_solid(_leds, NUM_LEDS, COL_OFF);
    FastLED.show();
}

void LedController::refresh(Mode mode, uint8_t selectedPatch, bool soloActive,
                             const bool* effectActive, bool bleConnected) {
    _bleConnected = bleConnected;

    if (!bleConnected) {
        // Handled in update() via blink — don't overwrite here
        return;
    }

    if (mode == Mode::NORMAL) {
        // SW_A1..SW_B2 (indices 0-3): patch select
        for (uint8_t i = 0; i <= SW_B2; i++) {
            _leds[i] = (i == selectedPatch) ? COL_ACTIVE_PATCH : COL_INACTIVE;
        }
        // SW_SOLO (4)
        _leds[SW_SOLO] = soloActive ? COL_SOLO_ON : COL_INACTIVE;
        // SW_MODE (5)
        _leds[SW_MODE] = COL_INACTIVE;
    } else {
        // EFFECTS mode: A1=BOOST, A2=MOD, B1=DELAY, B2=FX, SOLO=REVERB
        _leds[SW_A1]   = effectActive[CC_BOOST]  ? COL_FX_ON : COL_FX_OFF;
        _leds[SW_A2]   = effectActive[CC_MOD]    ? COL_FX_ON : COL_FX_OFF;
        _leds[SW_B1]   = effectActive[CC_DELAY]  ? COL_FX_ON : COL_FX_OFF;
        _leds[SW_B2]   = effectActive[CC_FX]     ? COL_FX_ON : COL_FX_OFF;
        _leds[SW_SOLO] = effectActive[CC_REVERB] ? COL_FX_ON : COL_FX_OFF;
        _leds[SW_MODE] = COL_MODE_EFFECTS;
    }

    FastLED.show();
}

void LedController::update() {
    if (_bleConnected) return;

    uint32_t now = millis();

    if (BleMidi.isScanning()) {
        // ── Knight Rider sweep in blue ────────────────────────────────────────
        if (now - _animTimer < SWEEP_STEP_MS) return;
        _animTimer = now;

        fill_solid(_leds, NUM_LEDS, COL_OFF);

        // Trail: two LEDs behind the head
        int8_t mid  = _sweepPos - _sweepDir;
        int8_t tail = _sweepPos - _sweepDir * 2;

        if (tail  >= 0 && tail  < NUM_LEDS) _leds[tail]  = COL_SWEEP_TAIL;
        if (mid   >= 0 && mid   < NUM_LEDS) _leds[mid]   = COL_SWEEP_MID;
        _leds[_sweepPos] = COL_SWEEP_HEAD;

        FastLED.show();

        _sweepPos += _sweepDir;
        if (_sweepPos >= NUM_LEDS) { _sweepPos = NUM_LEDS - 2; _sweepDir = -1; }
        if (_sweepPos < 0)         { _sweepPos = 1;             _sweepDir =  1; }

    } else {
        // ── Amber pulse while waiting to reconnect ────────────────────────────
        if (now - _animTimer < PULSE_STEP_MS) return;
        _animTimer = now;

        _pulseVal += _pulseDir;
        if (_pulseVal >= 120) { _pulseVal = 120; _pulseDir = -4; }
        if (_pulseVal <= 0)   { _pulseVal = 0;   _pulseDir =  4; }

        CRGB col = CRGB(
            (COL_WAIT_BASE.r * _pulseVal) / 120,
            (COL_WAIT_BASE.g * _pulseVal) / 120,
            0
        );
        fill_solid(_leds, NUM_LEDS, col);
        FastLED.show();
    }
}

void LedController::flash(CRGB color, uint8_t times, uint16_t ms) {
    for (uint8_t i = 0; i < times; i++) {
        fill_solid(_leds, NUM_LEDS, color);
        FastLED.show();
        delay(ms);
        fill_solid(_leds, NUM_LEDS, COL_OFF);
        FastLED.show();
        delay(ms);
    }
}
