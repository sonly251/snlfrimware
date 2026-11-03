# SNL GAMING v10.0

ESP32 firmware with games, apps, Wi-Fi tools, a virtual pet and an SD-card file manager.

## Games
- Snake
- Pong
- Racer
- Dino
- Shooter
- Flappy Bird
- Arkanoid
- Tanks
- Tetris
- DOOM
- Freeride
- Pac-Man
- Tower

## Apps
- Stopwatch
- Flashlight
- Calculator
- Random Generator
- Command Line
- Soundpad
- MP3 Player
- 3D Render
- Paint

## Other features
- Virtual pet
- Wi-Fi scan and connection
- Saved Wi-Fi networks
- Weather
- Web Remote
- SD-card file manager
- Text and HEX file viewer
- 128×64 bitmap viewer
- Video playback from SD card
- File information and deletion
- File upload over Wi-Fi
- Themes and records
- Device information
- Russian and English interface

## Firmware

Download the firmware matching your board from [Releases](../../releases).

| Board | File |
|---|---|
| Standard ESP32 | `SNL_GAMING_ESP32_v10.0_merged.bin` |
| ESP32 OLED variant | `SNL_GAMING_ESP32_OLED_v10.0_merged.bin` |

Both files are full 4 MB images. Flash the selected file at address `0x0000`.

## OLED pins

| Variant | SDA | SCL |
|---|---:|---:|
| Standard ESP32 | GPIO 21 | GPIO 22 |
| ESP32 OLED | GPIO 5 | GPIO 4 |

Use the firmware that matches your wiring. Flashing erases data stored in flash.

## Source
- `source/ESP32/` — standard ESP32
- `source/ESP32_OLED/` — alternate OLED wiring
