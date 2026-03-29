# BOM — Bill of Materials

Kusovník pro Boss Katana Footswitch. Ceny jsou orientační (AliExpress / TME / Laskakit).

---

## Hlavní součástky

| # | Název | Hodnota / Model | Množství | Cena / ks (Kč) | Celkem (Kč) | Poznámka |
|---|-------|----------------|----------|----------------|-------------|----------|
| 1 | ESP32-WROOM32 modul | 38-pin dev board | 1 | 150 | 150 | s USB-C nebo micro-USB |
| 2 | Momentary switch | viz AliExpress níže | 6 | 30 | 180 | objednaný switch |
| 3 | WS2812B LED | 5 mm nebo THT modul | 6 | 8 | 50 | nebo strip a rozstřihnout |
| 4 | Krabička | 120×70×40 mm | 1 | 200 | 200 | viz sekce krabičky |
| 5 | USB-C konektor (panel mount) | GX16 nebo USB-C | 1 | 40 | 40 | napájení |
| 6 | USB-C napájení 5V/1A | adaptér nebo powerbank | 1 | — | — | vlastní |

---

## Pasivní součástky

| # | Název | Hodnota | Množství | Cena (Kč) | Poznámka |
|---|-------|---------|----------|-----------|----------|
| 7 | Rezistor | 100 Ω, 1/4 W | 1 | 1 | data linka WS2812B |
| 8 | Elektrolytický kondenzátor | 100 µF / 10 V | 1 | 3 | stabilizace napájení LED |
| 9 | Keramický kondenzátor | 100 nF | 2 | 1 | bypass u ESP32 VCC/GND |

---

## Kabeláž a konektory

| # | Název | Specifikace | Množství | Cena (Kč) | Poznámka |
|---|-------|-------------|----------|-----------|----------|
| 10 | Drát pro switche | 0,25 mm², různé barvy | ~1 m | 30 | 7 žil (6 signál + GND) |
| 11 | Drát pro LED | 0,25 mm², 3 barvy | ~0,5 m | 15 | VCC, GND, DATA |
| 12 | Drát pro napájení | 0,5 mm² červený/černý | ~0,3 m | 10 | 5V/GND |
| 13 | Pájecí cín | 0,6–1,0 mm, Sn60Pb40 nebo bezolovnatý | 1 m | 15 | |
| 14 | Smršťovací bužírka | 2 mm, 4 mm | sada | 20 | izolace spojů |
| 15 | Stahovací pásky | 2,5×100 mm | 10 ks | 5 | uchycení kabelů |

---

## Volitelné — napájení z 9V DC

Pro napájení z klasické Boss PSA-230 zástrčky (9V center-positive):

| # | Název | Hodnota / Model | Množství | Cena (Kč) | Poznámka |
|---|-------|----------------|----------|-----------|----------|
| 16 | DC barrel jack | 2,1 mm, panel mount | 1 | 20 | 9V vstup |
| 17 | LDO regulátor | AMS1117-5V (SOT-223) | 1 | 5 | |
| 18 | El. kondenzátor (in) | 100 µF / 16 V | 1 | 4 | vstup regulátoru |
| 19 | El. kondenzátor (out) | 100 µF / 10 V | 1 | 3 | výstup regulátoru |
| 20 | Malá DPS / stripboard | 30×20 mm | 1 | 15 | pro regulátor |

---

## Nástroje a spotřební materiál

| Název | Poznámka |
|-------|----------|
| Pájecí stanice | min. 40 W, nastavitelná teplota |
| Vrtačka + vrtáky | 3 mm, 6 mm, 12 mm pro krabičku |
| Krokosvorky | pro testování před montáží |
| Multimetr | kontrola napětí a kontinuity |
| Flux (tavidlo) | usnadní pájení |
| Izopropylalkohol | čištění DPS po pájení |

---

## Odkazové zdroje

| Součástka | Zdroj |
|-----------|-------|
| Momentary switch | [AliExpress #1005004646906063](https://www.aliexpress.com/item/1005004646906063.html) |
| ESP32-WROOM32 | [Laskakit](https://www.laskakit.cz/esp32-wroom-32/) nebo AliExpress |
| WS2812B (volné LED) | [AliExpress — hledat "WS2812B 5mm THT"](https://www.aliexpress.com/w/wholesale-ws2812b-5mm-tht.html) |
| WS2812B (strip) | [Laskakit WS2812B strip](https://www.laskakit.cz/led-pasek-ws2812b/) |
| Pasivní součástky | [TME.eu](https://www.tme.eu) nebo [GME.cz](https://www.gme.cz) |

---

## Celkové náklady (orientačně)

| Kategorie | Kč |
|-----------|----|
| Elektronika (ESP32, switche, LED) | ~380 |
| Krabička | ~200 |
| Pasivní součástky + kabeláž | ~100 |
| **Celkem bez nástrojů** | **~680 Kč** |

> Ceny bez dopravy. AliExpress doprava zdarma, ale čekání 3–6 týdnů. Laskakit / TME — do 2 dnů.
