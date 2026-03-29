# Návrh Krabičky — Enclosure Design

## Rozložení tlačítek

Layout odpovídá fyzickému rozmístění na podlaze:

```
┌──────────────────────────────────────────────┐
│                                              │
│   ○ [A1]      ○ [A2]      ○ [SOLO]          │
│   ●  ──────   ●  ──────   ●  ──────         │
│  ( )          ( )          ( )               │
│                                              │
│   ○ [B1]      ○ [B2]      ○ [MODE]          │
│   ●  ──────   ●  ──────   ●  ──────         │
│  ( )          ( )          ( )               │
│                                              │
└──────────────────────────────────────────────┘
          ←──── 120 mm ────→
  ↑ 70 mm
```

`○` = LED
`●──` = popisek (štítek nebo gravírování)
`( )` = switch

---

## Doporučené rozměry krabičky

```
Délka:  120 mm  (3 switche × 30 mm rozteč + okraje)
Šířka:   70 mm  (2 řady × 25 mm + okraje)
Výška:   40 mm  (prostor pro ESP32 + kabeláž)
Tloušťka víka: 3–4 mm (hliník) nebo 4–5 mm (plast/3D tisk)
```

### Rozteče otvorů (středy)

```
Horizontálně:  30 mm mezi switchi (A1→A2→SOLO, B1→B2→MODE)
Vertikálně:    30 mm mezi řadami (A→B)

Střed první řady (A):  Y = 22 mm od spodní hrany víka
Střed druhé řady (B):  Y = 48 mm od spodní hrany víka

Středy X:
  Sloupec 1 (A1/B1):  X = 20 mm od levého okraje
  Sloupec 2 (A2/B2):  X = 50 mm
  Sloupec 3 (SOLO/MODE): X = 80 mm
```

### Průměr otvorů

| Otvor | Průměr |
|-------|--------|
| Switch (tělo) | dle switche — typicky **12 mm** |
| LED (5 mm THT) | **5,2 mm** |
| USB-C konektor | **10×4 mm** (obdélník) nebo dle konektoru |
| 9V DC barrel jack | **8 mm** |

---

## Možnosti materiálu

### 1. Hliníková krabička — doporučeno pro pedál

**Výhody:** odolná, profesionální vzhled, stínění EMI, tlumí vibrace
**Nevýhody:** vrtání otvorů je náročnější, cena vyšší

Konkrétní modely (běžně dostupné):

| Model | Rozměry | Zdroj | Cena |
|-------|---------|-------|------|
| Hammond 1590BB | 119×94×34 mm | GME, TME | ~250 Kč |
| Hammond 1590LB | 130×73×38 mm | GME, TME | ~270 Kč |
| Čínský klon 1590LB | 130×73×40 mm | AliExpress | ~120 Kč |
| Vlastní přesná rozteč | 120×70×40 mm | 3D tisk / fréza | — |

> **Tip:** Hammond 1590LB nebo jeho čínský klon jsou rozměrově ideální — 6 switchů se pohodlně vejde.

### 2. 3D tisk — nejflexibilnější

**Výhody:** přesné rozměry, libovolný tvar, integrované držáky pro PCB/ESP32
**Nevýhody:** méně odolný mechanicky, vhodné spíše pro prototyp

Doporučený materiál: **PETG** (odolnější než PLA) nebo **ABS**
Tloušťka stěn: min. 3 mm
Infill: 30–50 %

**Parametry pro tisk:**

```
Víko (horní část s otvory):
  - tloušťka 4 mm
  - otvory pro switche 12 mm (vrtá se nebo rovnou v modelu)
  - otvory pro LED 5,2 mm
  - zkosení 45° na hranách pro estetiku

Spodek (box):
  - integrované sloupky M3 pro uchycení ESP32 dev boardu
  - kabelová průchodka
  - protiskluzové nožky (gumové podložky nebo integrované výstupky)
```

### 3. Plastová rozvaděčová krabička

**Výhody:** levná, dostupná (GM Electronic, Hadex), rychlé řešení
**Nevýhody:** méně odolná při šlapání

| Model | Rozměry | Cena |
|-------|---------|------|
| Kradex Z-27 | 130×70×43 mm | ~60 Kč |
| Kradex Z-28 | 160×100×52 mm | ~80 Kč |

---

## Rozmístění komponent uvnitř krabičky

```
┌──────────────────────────────────────────────────────┐
│  ╔══════╗  ╔══════╗  ╔══════╗                        │
│  ║ SW_A1║  ║ SW_A2║  ║SW_SOL║   ← víko s vyfrézovanými │
│  ╚══════╝  ╚══════╝  ╚══════╝     otvory             │
│  ╔══════╗  ╔══════╗  ╔══════╗                        │
│  ║ SW_B1║  ║ SW_B2║  ║SW_MOD║                        │
│  ╚══════╝  ╚══════╝  ╚══════╝                        │
├──────────────────────────────────────────────────────┤
│                                                      │
│  ┌────────────────┐    Kabeláž ke switchům           │
│  │  ESP32-WROOM32 │    (svazek stáhnutý pásky)        │
│  │  dev board     │                                  │
│  └────────────────┘                                  │
│                                                      │
│  [USB-C]  nebo  [9V DC]                              │
└──────────────────────────────────────────────────────┘
          ↑ spodek krabičky (strana se šrouby)
```

### Uchycení ESP32

ESP32 dev board (38-pin) má montážní otvory. Možnosti:

1. **M3 distanční sloupky 10 mm** přilepené/přišroubované ke dnu krabičky
2. **3D tištěný nosník** (pokud tiskneš krabičku)
3. **Suchý zip** — jednoduchý, bez vrtání

---

## Gumové nožky

Pro pevný stoj na podlaze a ochranu před klouzáním:

- 4× gumová nožka ∅10 mm, samolepicí — AliExpress / GME, cca 5 Kč/ks
- Alternativa: proužky z protiskluzové podložky (IKEA VARIERA)

---

## Štítky / popis tlačítek

### Možnost A — průhledný štítek pod víkem (doporučeno)

Vytiskni štítek na papír/fólii, vlož pod průhledné akrylové víko nebo zalij pryskyřicí.

```
┌──────────────────────────────────────────────────────┐
│  [LED]  A1    [LED]  A2    [LED]  SOLO                │
│  [SW]         [SW]         [SW]                      │
│                                                      │
│  [LED]  B1    [LED]  B2    [LED]  MODE               │
│  [SW]         [SW]         [SW]                      │
└──────────────────────────────────────────────────────┘
```

### Možnost B — gravírování

Na hliníkový povrch laserem nebo ručně ryjecí jehlou.

### Možnost C — samolepicí štítky

Tisknutelné etikety (Avery, 63,5×38 mm), laminované pro odolnost.

---

## Hotové řešení — nákres víka (pro vrtání)

```
Víko 120 × 70 mm (pohled shora):

   10   20   30   40   50   60   70   80   90  100  110  120
 0 ┌─────────────────────────────────────────────────────────┐
   │                                                         │
10 │                                                         │
   │                                                         │
20 │              *                    *                    *│ ← Y=22 (LED 5,2mm)
   │           ▪▪▪▪▪▪▪            ▪▪▪▪▪▪▪           ▪▪▪▪▪▪▪ │
25 │          ▪       ▪          ▪       ▪         ▪       ▪ │
   │     SW_A1▪  12mm ▪    SW_A2 ▪  12mm ▪  SOLO   ▪  12mm ▪│ ← Y=30 (Switch 12mm)
30 │          ▪       ▪          ▪       ▪         ▪       ▪ │
   │           ▪▪▪▪▪▪▪            ▪▪▪▪▪▪▪           ▪▪▪▪▪▪▪ │
35 │                                                         │
   │                                                         │
40 │                                                         │
   │              *                    *                    *│ ← Y=48 (LED 5,2mm)
45 │           ▪▪▪▪▪▪▪            ▪▪▪▪▪▪▪           ▪▪▪▪▪▪▪ │
   │          ▪       ▪          ▪       ▪         ▪       ▪ │
50 │     SW_B1▪  12mm ▪    SW_B2 ▪  12mm ▪  MODE   ▪  12mm ▪│ ← Y=55 (Switch 12mm)
   │          ▪       ▪          ▪       ▪         ▪       ▪ │
55 │           ▪▪▪▪▪▪▪            ▪▪▪▪▪▪▪           ▪▪▪▪▪▪▪ │
   │                                                         │
60 │                                                         │
   │                                                         │
70 └─────────────────────────────────────────────────────────┘
   X=  20        50          80
```

Středy switch-otvorů: X = {20, 50, 80}, Y = {30, 55}
Středy LED-otvorů:    X = {20, 50, 80}, Y = {22, 48} (8 mm nad switchem)

### Postup vrtání hliníkového víka

1. Vytiskni šablonu na papír v měřítku 1:1 a přilep na víko páskou
2. Nastřed otvory důlčíkem
3. Předvrtej 3 mm
4. Zvětši na 5,2 mm (LED), pak na 10 mm, pak na 12 mm (switch) — stupňovitý vrták
5. Odeber jehly, přebrousit pilníčkem
6. Odstraň šablonu, otryskat nebo vyleštit povrch

---

## Doporučení

| Priorita | Volba |
|----------|-------|
| Nejrychlejší | Kradex Z-27 plastová krabička + samolepicí štítky |
| Nejrobustnější | Hammond 1590LB hliník + gravírování |
| Nejflexibilnější | 3D tisk PETG (vlastní nebo Printables.com model) |
| Nejlevnější | Čínský klon 1590 + štítek z tiskárny |
