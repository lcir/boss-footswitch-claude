# CLAUDE.md — Boss Katana BLE MIDI Footswitch

This file provides context for AI assistants working on this embedded C++ firmware project.

## Project Overview

A DIY 6-button Bluetooth MIDI footswitch for the **Boss Katana 50 Gen3** amplifier. The ESP32-based device connects via BLE MIDI to a Boss BT-Dual adapter and provides real-time RGB LED feedback and a web-based configuration UI.

**Key capabilities:**
- Two operational modes: Channel Mode (Program Change) and Effects Mode (Control Change)
- Auto-reconnecting BLE MIDI client
- WiFi provisioning via captive portal AP mode
- Real-time web interface with virtual pedal and WebSocket sync
- Configuration persisted to ESP32 NVS (non-volatile storage)

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Hardware | ESP32-WROOM32, 6x WS2812B LEDs, 6x momentary buttons |
| Language | C++ (Arduino framework) |
| Build | PlatformIO |
| BLE | NimBLE-Arduino (`h2zero/NimBLE-Arduino@^1.4.2`) |
| LEDs | FastLED (`fastled/FastLED@^3.7.0`) |
| HTTP | ESPAsyncWebServer + AsyncTCP |
| JSON | ArduinoJson v7 |
| Filesystem | LittleFS (web UI assets in `data/`) |

## Repository Structure

```
boss-footswitch-claude/
├── platformio.ini        # Build config (board, libs, flags)
├── src/                  # Firmware source (C++)
│   ├── main.cpp          # setup(), loop(), global event handlers
│   ├── config.h/.cpp     # Config management via ESP32 NVS
│   ├── footswitch.h/.cpp # Button debounce + mode state machine
│   ├── ble_midi_client.h/.cpp  # NimBLE MIDI client + auto-reconnect
│   ├── led_controller.h/.cpp   # WS2812B animations via FastLED
│   ├── wifi_manager.h/.cpp     # AP/STA WiFi + mDNS
│   └── web_interface.h/.cpp    # HTTP REST + WebSocket server
├── data/                 # LittleFS web UI assets
│   ├── index.html        # Single-page app (Czech language)
│   ├── app.js            # Frontend logic
│   └── style.css         # Styles
└── doc/                  # Documentation (Czech language)
    ├── README.md         # Main docs, wiring, setup guide
    ├── wiring.md         # GPIO/LED pin assignments
    ├── enclosure.md      # 3D print specs
    └── bom.md            # Bill of materials
```

## Architecture

### Component Roles

Each module is a singleton initialized in `setup()` and coordinated via callbacks registered in `main.cpp`.

| Component | Header | Global Instance | Role |
|-----------|--------|-----------------|------|
| Config | `config.h` | `Config` | Load/save settings from NVS |
| Footswitch | `footswitch.h` | `Pedal` | Debounce buttons, emit events |
| BLE MIDI | `ble_midi_client.h` | `BleMidi` | BLE client, MIDI encoding |
| LED Controller | `led_controller.h` | `Leds` | RGB feedback & animations |
| WiFi Manager | `wifi_manager.h` | `WifiMgr` | Network provisioning |
| Web Interface | `web_interface.h` | `WebUI` | HTTP/WebSocket server |

### Event Flow on Button Press

```
GPIO (INPUT_PULLUP) → Footswitch::update() [30ms debounce]
  → emits FootswitchEvent
    → main::onFootswitchEvent()
        ├── BleMidi.sendProgramChange() / sendControlChange()
        ├── Leds.refresh()
        └── WebUI.broadcastState()   ← WebSocket to all clients
```

### BLE MIDI Reconnection

1. On disconnect: schedule passive BLE scan every 5 seconds
2. Scan for configurable device name (default: `"KATANA 3 MIDI"`)
3. Connect via NimBLE callbacks; bond with "Just Works" security
4. Config allows bonding up to 3 devices (`-DCONFIG_BT_NIMBLE_MAX_BONDS=3`)

### Configuration Storage

Settings are stored via the ESP32 `Preferences` library (NVS):
- `midi_channel` (1–16, default 1)
- `pc_A1`, `pc_A2`, `pc_B1`, `pc_B2` — Program Change values (0–127)
- `cc_solo`, `cc_boost`, `cc_mod`, `cc_delay`, `cc_fx`, `cc_reverb` — CC numbers
- `ble_device_name` — BLE scan target
- WiFi credentials (SSID/password)

Web API: `POST /api/config` with JSON body; `GET /api/config` to read.

### Operational Modes

| Mode | Button A1/A2/B1/B2 | Button SOLO | Button MODE |
|------|-------------------|-------------|-------------|
| Channel | Program Change (patches) | CC#73 | Switch to Effects |
| Effects | CC#80–83 (boost/mod/delay/fx) | CC#84 (reverb) | Switch to Channel |

Mode switching triggers a sweep LED animation (orange → Channel, green → Effects).

## Build & Flash Workflows

### Prerequisites

- PlatformIO CLI or VS Code PlatformIO extension
- ESP32 connected via USB-to-serial adapter

### Common Commands

```bash
# Compile firmware
pio run

# Compile + flash firmware
pio run --target upload

# Flash web UI assets to LittleFS
pio run --target uploadfs

# Open serial monitor (115200 baud)
pio device monitor

# Clean build artifacts
pio run --target clean
```

### First-Time Setup on New Device

1. Flash LittleFS first: `pio run --target uploadfs`
2. Flash firmware: `pio run --target upload`
3. On boot: device starts AP mode (`BossFootswitch` SSID) if no WiFi is saved
4. Connect to AP and navigate to captive portal to configure WiFi
5. Device restarts in STA mode; accessible at `http://boss-footswitch.local`

## Development Conventions

### Code Style

- Arduino C++ idioms (no STL, prefer stack/global allocation)
- Module pattern: each component has a `.h` (declarations) and `.cpp` (implementation)
- Callbacks registered in `main.cpp` to decouple components — avoid cross-module direct calls
- Debug output via `Serial.printf()` with `[MODULE]` prefix tags
- No dynamic memory allocation in hot paths (ISRs, LED updates)

### Adding a New Module

1. Create `src/mymodule.h` and `src/mymodule.cpp`
2. Declare a global instance (e.g., `extern MyModule MyMod;`) in the header
3. Define the global in `mymodule.cpp`; initialize in `setup()` in `main.cpp`
4. Register callbacks in `main.cpp` rather than calling other modules directly

### Modifying BLE Behavior

- BLE logic lives in `ble_midi_client.cpp`
- NimBLE callbacks: `onConnect`, `onDisconnect`, `onResult` (scan)
- MIDI messages are encoded as BLE MIDI packets (timestamp + status + data bytes)
- The BLE service UUID is `03B80E5A-EDE8-4B33-A751-6CE34EC4C700` (standard BLE MIDI)

### Modifying Web UI

- Edit files in `data/` (HTML/CSS/JS)
- After any change, reflash LittleFS: `pio run --target uploadfs`
- WebSocket endpoint: `ws://<device-ip>/ws`
- REST endpoints:
  - `GET /api/config` — returns JSON config
  - `POST /api/config` — updates and saves config
  - `GET /api/state` — returns current pedal state

### Modifying LED Animations

- All LED logic is in `led_controller.cpp`
- LEDs are WS2812B on a single data pin (see `doc/wiring.md` for GPIO)
- FastLED is used; color constants defined in `led_controller.h`
- Animations run via non-blocking timers in `loop()` — do not use `delay()`

## Debugging

### Serial Debug Output

```bash
pio device monitor  # 115200 baud
```

Log tags to look for:
- `[BLE]` — BLE scanning, connect/disconnect events, MIDI sends
- `[WIFI]` — AP/STA transitions, IP assignment
- `[WEB]` — HTTP requests, WebSocket events
- `[SW]` — Button presses, mode changes

### Web UI Log Viewer

The web interface at `http://boss-footswitch.local` includes a BLE log panel showing the last 30 events in real time.

### Build Flags

Defined in `platformio.ini`:
- `-DCORE_DEBUG_LEVEL=3` — verbose ESP-IDF debug output (change to `0` for production)
- `-DCONFIG_BT_NIMBLE_MAX_BONDS=3` — max BLE bonded devices

## Documentation Language

All end-user documentation in `doc/` and the web UI (`data/`) are in **Czech**. Code comments are minimal and primarily in English. When adding new user-facing strings to the web UI, follow the existing Czech language convention.

## No Test Suite

There are no automated tests. Validation is manual:
- Physical button presses observed via serial monitor and LEDs
- Virtual buttons on the web interface for BLE MIDI testing
- Serial log inspection for BLE/WiFi events

When making changes, manually verify:
1. Firmware compiles without warnings: `pio run`
2. Expected Serial log output after flash
3. Web UI loads and WebSocket connects
4. BLE reconnects after power cycle

## Key Constraints

- **No blocking calls in `loop()`** — use non-blocking timers and callbacks
- **LittleFS size** — web assets must fit within the LittleFS partition (~1.5 MB)
- **NimBLE + WiFi coexistence** — both use the ESP32 radio; avoid simultaneous heavy activity
- **Single MIDI channel** — all messages go on the same configurable MIDI channel
- **Arduino `delay()` is forbidden** in any module except one-time setup sequences
