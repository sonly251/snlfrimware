# SNLGAMING ESP32

ESP32 handheld gaming firmware with a 128x64 SSD1306 OLED interface, games, apps, virtual pet, EEPROM storage and DFPlayer Mini support.

## Two firmware versions

This repository contains both firmware variants in one repository:

| Version | Folder | OLED SDA/SCL | Buttons |
|---|---|---|---|
| **SNLGAMING-ESP32** | `firmware/SNLGAMING-ESP32/` | GPIO 21 / 22 | UP 27, DOWN 26, SELECT 25, BACK 14, LEFT 32, RIGHT 33 |
| **SNLGAMING-ESP32-OLED** | `firmware/SNLGAMING-ESP32-OLED/` | GPIO 5 / 4 | UP 13, DOWN 12, SELECT 15, BACK 16, LEFT 14, RIGHT 2 |

Both source files use a 128x64 SSD1306 display and I2C address `0x3C`.

### Important hardware warning

In the **SNLGAMING-ESP32-OLED** source, `BTN_BACK` is GPIO16 and `DFPLAYER_RX` is also GPIO16. If your hardware uses DFPlayer Mini, this is a GPIO conflict. Do not wire both functions to GPIO16 without changing the firmware pin assignment and the corresponding wiring.

## Features

### Games
- Snake
- Pong
- Racer
- Dino
- Shooter
- Flappy Bird
- Arkanoid
- Tanks
- Tetris

### Apps
- Stopwatch
- Flashlight
- Calculator
- Random Generator
- Command Line
- SoundPad
- MP3 Player
- 3D Render

## Hardware

### Common
- ESP32
- SSD1306 OLED 128x64, I2C
- 6 buttons
- Speaker on GPIO23
- Optional DFPlayer Mini
- DFPlayer UART: RX GPIO16, TX GPIO17 in the source
- 20 MP3 tracks are configured

### Version 1: SNLGAMING-ESP32

| Function | GPIO |
|---|---:|
| OLED SDA | 21 |
| OLED SCL | 22 |
| UP | 27 |
| DOWN | 26 |
| SELECT | 25 |
| BACK | 14 |
| LEFT | 32 |
| RIGHT | 33 |
| Speaker | 23 |
| DFPlayer RX | 16 |
| DFPlayer TX | 17 |

### Version 2: SNLGAMING-ESP32-OLED

| Function | GPIO |
|---|---:|
| OLED SDA | 5 |
| OLED SCL | 4 |
| UP | 13 |
| DOWN | 12 |
| SELECT | 15 |
| BACK | 16 |
| LEFT | 14 |
| RIGHT | 2 |
| Speaker | 23 |
| DFPlayer RX | 16 |
| DFPlayer TX | 17 |

## Arduino IDE

1. Install Arduino IDE.
2. Open **File → Preferences**.
3. Add this URL to **Additional Boards Manager URLs**:

   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`

4. Open **Tools → Board → Boards Manager**.
5. Search for `esp32`.
6. Install **esp32 by Espressif Systems**.
7. Install these libraries from **Library Manager**:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - DFRobotDFPlayerMini

## How to open each firmware

Do not put both `.ino` files into the same Arduino sketch folder.

Open:

`firmware/SNLGAMING-ESP32/SNLGAMING-ESP32.ino`

or:

`firmware/SNLGAMING-ESP32-OLED/SNLGAMING-ESP32-OLED.ino`

Then select your ESP32 board and COM/serial port and use **Verify** / **Upload**.

## Repository structure

```text
SNLGAMING-ESP32/
├── README.md
├── .gitignore
├── docs/
│   └── wiring.md
└── firmware/
    ├── SNLGAMING-ESP32/
    │   └── SNLGAMING-ESP32.ino
    └── SNLGAMING-ESP32-OLED/
        └── SNLGAMING-ESP32-OLED.ino
```

## MP3

The firmware configures 20 MP3 tracks and initializes DFPlayer Mini through `Serial2` at 9600 baud.

## EEPROM

The firmware stores game records and settings in EEPROM, including game high scores, theme/settings, pet data, boot settings and Tetris high score.

## Controls

The six buttons are configured with `INPUT_PULLUP`, so the button input is active LOW.

## Command line

The firmware includes a command-line application with commands such as:

```text
snldo help
snldo info
snldo games
snldo apps
snldo pet
snldo records
snldo theme
snldo reboot
snldo code
```

## License

No license is declared in this repository. Add a license only if you have decided which license applies to your code and any included assets.
