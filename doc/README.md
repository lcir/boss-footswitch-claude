# Boss Katana Footswitch — Dokumentace

Vlastnoruční Bluetooth MIDI footswitch pro Boss Katana 50 Gen3 s adaptérem BT-Dual.
Řídicí jednotka je ESP32-WROOM32 s 6 momentary switchi a 6 WS2812B RGB LED.

---

## Obsah

- [Hardware](#hardware)
- [Zapojení](#zapojení)
- [Firmware — build a flash](#firmware--build-a-flash)
- [První spuštění a WiFi](#první-spuštění-a-wifi)
- [Ovládání — fyzický footswitch](#ovládání--fyzický-footswitch)
- [Webové rozhraní](#webové-rozhraní)
- [MIDI konfigurace](#midi-konfigurace)
- [Architektura kódu](#architektura-kódu)
- [LED barvy a stavový přehled](#led-barvy-a-stavový-přehled)
- [Řešení problémů](#řešení-problémů)

---

## Hardware

| Součástka | Detail |
|-----------|--------|
| MCU | ESP32-WROOM32 |
| Switche | 6× momentary push button ([AliExpress #1005004646906063](https://www.aliexpress.com/item/1005004646906063.html)) |
| LED | 6× WS2812B (NeoPixel) — daisy chain |
| Napájení | 5 V přes USB-C |
| Komunikace | BLE MIDI → Boss BT-Dual → Katana 50 Gen3 |

---

## Zapojení

```
ESP32-WROOM32
┌───────────────────────────────────┐
│  GPIO 4   ──── SW1 (A1)  ──┐      │
│  GPIO 5   ──── SW2 (A2)    │      │
│  GPIO 18  ──── SW3 (B1)    ├─ GND │
│  GPIO 19  ──── SW4 (B2)    │      │
│  GPIO 21  ──── SW5 (SOLO)  │      │
│  GPIO 22  ──── SW6 (MODE) ─┘      │
│                                   │
│  GPIO 23  ──── WS2812B DATA       │
│  5V       ──── WS2812B VCC        │
│  GND      ──── WS2812B GND        │
└───────────────────────────────────┘
```

### Switche

Každý switch se zapojí mezi příslušný GPIO pin a GND.
Interní pull-up (`INPUT_PULLUP`) je povolen v kódu — žádný externí rezistor není potřeba.

### WS2812B LED

LED jsou zapojeny do série (daisy chain). Datový výstup každé LED jde na datový vstup další.

```
GPIO 23 → DIN[LED1] → DOUT[LED1] → DIN[LED2] → … → DIN[LED6]
```

Pořadí LED odpovídá pořadí switchů: LED1=A1, LED2=A2, LED3=B1, LED4=B2, LED5=SOLO, LED6=MODE.

> **Poznámka k napájení:** WS2812B odebírají při plném bílém světle ~60 mA každá (360 mA celkem).
> Při napájení z USB ESP32 (500 mA) je to v pořádku, ale jas je v kódu omezen na 120/255.
> Pro delší kabely doporučuji přidat 100–470 Ω sériový rezistor na datový vodič a 100 µF kondenzátor na VCC/GND blízko prvního LED.

---

## Firmware — build a flash

### Požadavky

- [Visual Studio Code](https://code.visualstudio.com/) + [PlatformIO rozšíření](https://platformio.org/install/ide?install=vscode)
- nebo PlatformIO CLI: `pip install platformio`

### Build

```bash
cd boss-footswitch-claude
pio run
```

### Flash firmware

```bash
pio run --target upload
```

### Flash webových souborů (LittleFS)

```bash
pio run --target uploadfs
```

> **Důležité:** `uploadfs` se musí provést **jednou po prvním flashování** (nebo po změně souborů v `data/`). Bez toho webové rozhraní nebude fungovat.

### Sériový monitor (debug výpisy)

```bash
pio device monitor
```

---

## První spuštění a WiFi

1. ESP32 nastartuje a zjistí, že nemá uložené WiFi přihlašovací údaje.
2. Vytvoří Access Point s názvem **`BossFootswitch-Setup`** (bez hesla).
3. Připoj se k tomuto AP z telefonu nebo počítače.
4. Prohlížeč by měl automaticky otevřít setup stránku (captive portal). Pokud ne, otevři `192.168.4.1`.
5. Zadej název sítě (SSID) a heslo.
6. Po kliknutí na *Connect* se ESP32 restartuje a připojí k zadané síti.
7. Web rozhraní je pak dostupné na **`http://boss-footswitch.local`** (nebo na IP adrese z routeru).

---

## Ovládání — fyzický footswitch

### Normální režim (Channel Mode)

| Tlačítko | Funkce | MIDI zpráva |
|----------|--------|-------------|
| **A1** | Přepni na patch A1 | Program Change PC#0 |
| **A2** | Přepni na patch A2 | Program Change PC#1 |
| **B1** | Přepni na patch B1 | Program Change PC#4 |
| **B2** | Přepni na patch B2 | Program Change PC#5 |
| **SOLO** | Zapni/vypni Solo boost | CC#73, hodnota 0/127 |
| **MODE** | Přepni do Effects Mode | — |

### Effects režim (Effects Mode)

| Tlačítko | Efekt | MIDI zpráva |
|----------|-------|-------------|
| **A1** | Boost on/off | CC#80, hodnota 0/127 |
| **A2** | Mod (chorus/flanger/…) on/off | CC#81, hodnota 0/127 |
| **B1** | Delay on/off | CC#82, hodnota 0/127 |
| **B2** | FX on/off | CC#83, hodnota 0/127 |
| **SOLO** | Reverb on/off | CC#84, hodnota 0/127 |
| **MODE** | Zpět do Channel Mode | — |

> Výchozí čísla PC a CC odpovídají výchozímu nastavení Boss Tone Studio.
> Pokud máš jiné mapování, uprav ho ve webovém rozhraní → MIDI Settings.

---

## Webové rozhraní

Dostupné na `http://boss-footswitch.local` po připojení k WiFi.

### Virtuální footswitch

Zobrazuje 6 tlačítek se stejným chováním jako fyzický pedál. Stav (aktivní channel, zapnuté efekty) se synchronizuje v reálném čase přes WebSocket.

### Status bar

| Indikátor | Zelený | Červený |
|-----------|--------|---------|
| **BLE** | Připojeno k BT-Dual | Odpojeno / hledá |
| **Web** | WebSocket aktivní | Odpojeno |

### MIDI Settings

Rozbalitelná sekce na dně stránky. Umožňuje změnit:

- MIDI kanál (1–16, výchozí: 1)
- Program Change čísla pro patcheA1/A2/B1/B2
- CC čísla pro efekty (Solo, Boost, Mod, Delay, FX, Reverb)

Po kliknutí na **Save Settings** se hodnoty uloží do flash paměti (NVS) a jsou aktivní okamžitě.

---

## MIDI konfigurace

### Nastavení v Boss Tone Studio

1. Připoj Katanu k PC přes USB.
2. Otevři Boss Tone Studio → System → MIDI Settings.
3. Nastav MIDI RX Channel na **1** (nebo na kanál nastavený v footswitch).
4. Pro efekty: přiřaď CC čísla shodná s nastavením footswitche (viz tabulka výše).
5. Pro channel switching: ulož patchy do paměťových pozic 1–8 (odpovídají PC#0–7).

### Výchozí CC mapování (Katana Gen3)

| Efekt | Výchozí CC# |
|-------|-------------|
| Solo boost | 73 |
| Boost | 80 |
| Mod | 81 |
| Delay | 82 |
| FX | 83 |
| Reverb | 84 |

> CC čísla nejsou oficálně zdokumentována Bossem. Výše uvedené hodnoty jsou komunitou ověřené výchozí hodnoty pro Tone Studio. Pokud efekty nereagují, ověř mapování v Tone Studio a uprav hodnoty ve webovém rozhraní.

---

## Architektura kódu

```
src/
├── main.cpp              Vstupní bod: setup(), loop(), event handlers
├── config.h/.cpp         Konfigurace: NVS, výchozí MIDI hodnoty, AppConfig struct
├── ble_midi_client.h/.cpp BLE MIDI klient (NimBLE central → BT-Dual)
├── footswitch.h/.cpp     Debounce switchů, stavový automat Normal/Effects
├── led_controller.h/.cpp WS2812B LED řídič (FastLED)
├── wifi_manager.h/.cpp   WiFi provisioning (AP captive portal → STA), mDNS
└── web_interface.h/.cpp  HTTP server (ESPAsyncWebServer) + WebSocket /ws

data/                     LittleFS souborový systém (web UI)
├── index.html
├── style.css
└── app.js
```

### Datový tok při stisku tlačítka

```
GPIO interrupt (digitalRead)
  └─ Footswitch::update() → debounce → handlePress()
       └─ FootswitchEvent emitted
            ├─ main.cpp::onFootswitchEvent()
            │    ├─ BleMidi.sendProgramChange() nebo sendControlChange()
            │    └─ Leds.refresh()
            └─ WebUI.broadcastState() → WebSocket → prohlížeč
```

### BLE reconnect logika

```
BleMidiClient::update() — volá se z loop()
  └─ if (!connected && timer > 5s):
       startScan() → NimBLE scan 4s
         └─ onResult(): najde BLE-MIDI service UUID
              └─ connect() → discover service/characteristic
                   └─ ClientCB::onConnect() → _connected = true
```

---

## LED barvy a stavový přehled

| Stav | Barva |
|------|-------|
| Neaktivní tlačítko (Normal mode) | Tlumená bílá |
| Aktivní patch (Normal mode) | Zelená |
| SOLO aktivní | Červená |
| Efekt vypnutý (Effects mode) | Tlumená modrá |
| Efekt zapnutý (Effects mode) | Azurová (cyan) |
| MODE tlačítko v Effects mode | Oranžová |
| BLE odpojeno | Všechny LED: pomalé červené blikání |
| Boot | 2× modrý záblesk |
| BLE připojeno | 2× zelený záblesk |
| WiFi připojeno | 3× modrý záblesk |
| Přepnutí režimu | 1× záblesk (oranžový → effects, zelený → normal) |

---

## Řešení problémů

### BLE se nepřipojuje k BT-Dual

- Ujisti se, že BT-Dual je zasunutý v Katana a Katana je zapnutá.
- BT-Dual bliká rychle = čeká na spárování; pomalu = spárovaný ale odpojený.
- V sériovém monitoru sleduj výpisy `[BLE] Scanning…` a `[BLE] Found BLE-MIDI device`.
- Pokud ESP32 najde zařízení ale nepřipojí se, zkus resetovat BT-Dual (zasunutí/vysunutí).

### Efekty nereagují

- Ověř, že MIDI RX Channel v Tone Studio odpovídá nastavení v MIDI Settings.
- Ověř CC čísla v Tone Studio → přiřaď je shodně ve webovém rozhraní.
- Zkontroluj sériový monitor: `[MIDI] CC ch=1 cc=80 val=127` musí být vidět při stisku.

### Web rozhraní nedostupné

- Proveď `pio run --target uploadfs` — LittleFS soubory musí být nahrány zvlášť.
- Zkontroluj, že ESP32 je na stejné WiFi síti.
- Zkus IP adresu místo `boss-footswitch.local` (mDNS nemusí fungovat na Androidu).

### Switche reagují chaoticky

- Zkontroluj zapojení (GPIO na GND přes switch).
- Debounce je nastaven na 30 ms v `footswitch.h` (`DEBOUNCE_MS`). Zvyš hodnotu při problémech.

### WiFi nelze zadat (AP stránka se neotevře)

- Připoj se na SSID `BossFootswitch-Setup` a manuálně otevři `192.168.4.1`.
- Captive portal DNS funguje automaticky na iOS/macOS; na Androidu může být potřeba manuální navigace.

### Reset WiFi (zapomenutá síť)

Přidej do `setup()` dočasně: `Config::saveWifi("", ""); ESP.restart();` — ESP32 se přepne zpět do AP módu.
