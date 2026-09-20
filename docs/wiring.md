# Wiring

## Version 1 — SNLGAMING-ESP32

### OLED SSD1306 128x64 I2C

| OLED | ESP32 |
|---|---|
| VCC | 3.3V (according to your display module requirements) |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

### Buttons

The firmware uses `INPUT_PULLUP`, so each button should connect its GPIO input to GND when pressed.

| Button | ESP32 |
|---|---:|
| UP | GPIO27 |
| DOWN | GPIO26 |
| SELECT | GPIO25 |
| BACK | GPIO14 |
| LEFT | GPIO32 |
| RIGHT | GPIO33 |

### Audio

| Function | ESP32 |
|---|---:|
| Speaker | GPIO23 |
| DFPlayer RX | GPIO16 |
| DFPlayer TX | GPIO17 |

---

## Version 2 — SNLGAMING-ESP32-OLED

### OLED SSD1306 128x64 I2C

| OLED | ESP32 |
|---|---|
| VCC | 3.3V (according to your display module requirements) |
| GND | GND |
| SDA | GPIO5 |
| SCL | GPIO4 |

### Buttons

| Button | ESP32 |
|---|---:|
| UP | GPIO13 |
| DOWN | GPIO12 |
| SELECT | GPIO15 |
| BACK | GPIO16 |
| LEFT | GPIO14 |
| RIGHT | GPIO2 |

### Audio

| Function | ESP32 |
|---|---:|
| Speaker | GPIO23 |
| DFPlayer RX | GPIO16 |
| DFPlayer TX | GPIO17 |

### IMPORTANT

This version assigns **GPIO16 to both BACK and DFPlayer RX** in the source code. That is a hardware conflict if both are connected.

Before using DFPlayer Mini with this version, choose a different free GPIO for one of these functions, change the corresponding `#define` in the `.ino` file, and wire the hardware to match.

Do not assume that moving only the wire is enough—the firmware pin definition must match the wiring.
