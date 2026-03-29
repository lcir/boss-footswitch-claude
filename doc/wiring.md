# Zapojení — Wiring Guide

## Schéma zapojení (ASCII)

```
                        ┌─────────────────────────────────────┐
                        │         ESP32-WROOM32               │
                        │                                     │
    SW_A1  ────────────►│ GPIO4          GPIO23 ─────────────►│ WS2812B LED #1 DIN
    SW_A2  ────────────►│ GPIO5                               │
    SW_B1  ────────────►│ GPIO18                              │
    SW_B2  ────────────►│ GPIO19                              │
    SW_SOLO────────────►│ GPIO21                              │
    SW_MODE────────────►│ GPIO22                              │
                        │                                     │
              GND ──────┤ GND            5V ─────────────────►│ VCC → WS2812B
              5V  ──────┤ 5V              3.3V                │
                        └─────────────────────────────────────┘
```

---

## Switche

Každý switch se zapojí **mezi GPIO pin a GND**. Interní pull-up je povolen v kódu — žádný externí rezistor není potřeba.

```
GPIO_PIN ──┬── [Switch NO] ── GND
           │
           └── (INPUT_PULLUP, logická 1 = uvolněno, 0 = stisknuto)
```

### Pinout pro 6 switchů

| Switch | Label | GPIO | Barva vodiče (návrh) |
|--------|-------|------|----------------------|
| SW1 | A1 | 4 | bílá |
| SW2 | A2 | 5 | žlutá |
| SW3 | B1 | 18 | zelená |
| SW4 | B2 | 19 | modrá |
| SW5 | SOLO | 21 | červená |
| SW6 | MODE | 22 | oranžová |
| — | GND (společný) | GND | černá |

> **Tip:** Použij 7-žilový kabel (6 signálů + GND). Pokud switche mají LED, potřebuješ další vodiče — viz sekce LED.

---

## WS2812B LED

LED jsou zapojeny do **daisy-chain** série. Datový výstup každé LED (`DOUT`) jde na datový vstup (`DIN`) další.

```
GPIO23 ──[100Ω]── DIN[LED1] ── DOUT[LED1] ── DIN[LED2] ── DOUT[LED2] ── … ── DIN[LED6]

5V  ─────────────────────────────────────────────────────────────────── VCC (všechny LED)
GND ─────────────────────────────────────────────────────────────────── GND (všechny LED)
```

### Ochranné prvky

```
                         100 µF
GPIO23 ──[100Ω]──┬───────────────┬── DIN[LED1] …
                 │    ┴          │
                 │    ─ (100µF)  │
                 │               │
 5V ─────────────┼───────────────┼── VCC
                 │               │
GND ─────────────┴───────────────┴── GND
```

| Součástka | Hodnota | Účel |
|-----------|---------|------|
| Sériový rezistor | 100–470 Ω | Chrání datový pin ESP32 před přepětím a tlumí ringing |
| Kondenzátor | 100 µF / 10 V | Blokuje napěťové špičky při přepnutí WS2812B |

> Kondenzátor umísti co nejblíže **prvnímu** WS2812B (mezi VCC a GND).

### Pořadí LED vs. switch

| Index FastLED | Switch | Pozice na pedálu |
|---------------|--------|-----------------|
| 0 | SW_A1 | vlevo nahoře |
| 1 | SW_A2 | uprostřed nahoře |
| 2 | SW_B1 | vlevo dole |
| 3 | SW_B2 | uprostřed dole |
| 4 | SW_SOLO | vpravo nahoře |
| 5 | SW_MODE | vpravo dole |

---

## Napájení

ESP32-WROOM32 + 6× WS2812B při maximálním jasu:

| Zdroj | Proud | Poznámka |
|-------|-------|----------|
| ESP32 | ~240 mA | WiFi aktivní |
| 6× WS2812B (plný bílý) | ~360 mA | 60 mA/kus |
| **Celkem** | **~600 mA** | |

Jas v kódu omezen na **120/255 (~47 %)** → reálný příkon WS2812B cca 170 mA → celkem **~410 mA**.

### Doporučené napájení

- **USB-C 5V / 1A** — postačující, nejjednodušší řešení
- **9V DC center-positive + AMS1117-5V regulátor** — pokud chceš napájet z klasické 9V kytarové zástrčky (Boss PSA-230)

```
Možnost A — USB-C:
  USB-C 5V ──── ESP32 5V pin + WS2812B VCC

Možnost B — 9V DC jack:
  9V DC ──── AMS1117-5V ──── 5V ──── ESP32 5V pin + WS2812B VCC
              (100µF na vstupu, 100µF na výstupu)
```

---

## Kompletní seznam spojů

| Od | Do | Barva / poznámka |
|----|-----|-----------------|
| ESP32 GPIO4 | SW_A1 pin 1 | signál |
| ESP32 GPIO5 | SW_A2 pin 1 | signál |
| ESP32 GPIO18 | SW_B1 pin 1 | signál |
| ESP32 GPIO19 | SW_B2 pin 1 | signál |
| ESP32 GPIO21 | SW_SOLO pin 1 | signál |
| ESP32 GPIO22 | SW_MODE pin 1 | signál |
| GND (svorkovnice) | všechny SW pin 2 | společný GND |
| ESP32 GPIO23 | 100Ω rezistor → LED1 DIN | data WS2812B |
| LED1 DOUT | LED2 DIN | data chain |
| LED2 DOUT | LED3 DIN | data chain |
| LED3 DOUT | LED4 DIN | data chain |
| LED4 DOUT | LED5 DIN | data chain |
| LED5 DOUT | LED6 DIN | data chain |
| 5V | ESP32 5V, LED VCC (všechny) | napájení |
| GND | ESP32 GND, LED GND (všechny) | napájení |
