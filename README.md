# SNL GAMING — v10.0

**SNL GAMING** is an ESP32-based handheld mini-console and utility firmware with a compact graphical interface, built-in games, everyday tools, Wi-Fi features and file management.

This repository contains two hardware/wiring variants:

- **Standard ESP32**
- **ESP32 OLED wiring variant**

> Choose the firmware that matches your board and display wiring. The two variants are not interchangeable if their OLED wiring differs.

---

## ✨ Features

### 🎮 Games

The firmware includes the following games:

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
- **Freeride**
- **Pac-Man**
- **Tower**

Game records/high scores are available from the Records section. The firmware stores supported game and pet data in non-volatile memory.

### 🧰 Applications

The Apps menu includes:

- **Stopwatch** — a built-in stopwatch.
- **Flashlight** — uses the device display as a light.
- **Calculator** — an on-device calculator.
- **Random Generator** — generates random values.
- **Command Line** — built-in command interface for device commands and information.
- **Soundpad** — sound playback controls.
- **MP3 Player** — MP3 playback support.
- **3D Render** — a simple 3D rendering demo.

### 🐾 Virtual pet

A virtual pet with persistent data, including its level, mood and hunger. Interact with the pet and keep track of its progress.

### 📡 Wi-Fi and network tools

- Wi-Fi network scanning and connection.
- Save networks for later connection.
- Option to disable Wi-Fi.
- Weather information using online weather services (requires an internet connection).
- Web Remote: control the device from a browser over Wi-Fi.
- Wi-Fi file upload to the device through the file manager.

### 📁 File manager

The built-in file manager supports browsing files and folders on supported storage:

- LittleFS internal filesystem.
- SD card.
- Read-only flash-partition inspection.
- Text and HEX file viewing.
- Image viewing for supported bitmap files.
- Copy, move, paste, rename and delete operations where supported.
- Upload files over Wi-Fi.

Some file-manager functions require compatible SD-card hardware and/or storage to be connected.

### 🎨 Paint

A built-in drawing application for creating images on the device.

### ⚙️ Settings and interface

- Main menu with Games, Settings, Apps and Pet sections.
- Theme selection.
- Records/high-score screen.
- Device information/About screen.
- Code viewer.
- Boot music toggle.
- Boot speed setting.
- Russian and English interface text are included in the firmware.

---

## 📦 Firmware downloads

Download the `.bin` file that matches your hardware from the GitHub **Releases** page.

| Hardware variant | Firmware file |
|---|---|
| Standard ESP32 | `SNL_GAMING_ESP32_v10.0_merged.bin` |
| ESP32 OLED wiring variant | `SNL_GAMING_ESP32_OLED_v10.0_merged.bin` |

Only the two complete merged firmware images are distributed. There are no `app-only` firmware files in this release.

### Flashing

The merged images are full 4 MB flash images. They include the bootloader, partition table, `boot_app0` and application image.

1. Download the correct merged `.bin` for your hardware variant.
2. Connect the ESP32 to your computer over USB.
3. Use an ESP32 flashing tool that supports writing a full merged image.
4. Select the correct serial port.
5. Set the flash address to **`0x0000`** and flash the image.

**Important:** Flashing the wrong hardware variant may cause the display or controls to work incorrectly. Flashing can erase data stored in flash, so back up anything important first.

---

## 🔌 OLED wiring variants

The source archives identify these I²C pins:

| Variant | SDA | SCL |
|---|---:|---:|
| Standard ESP32 | GPIO 21 | GPIO 22 |
| ESP32 OLED wiring variant | GPIO 5 | GPIO 4 |

Check your actual board wiring before selecting a firmware image.

---

## 🗂️ Repository structure

```text
.
├── source/
│   ├── ESP32/          # Standard ESP32 source
│   └── ESP32_OLED/     # Alternate OLED wiring source
├── firmware/
│   ├── SNL_GAMING_ESP32_v10.0_merged.bin
│   └── SNL_GAMING_ESP32_OLED_v10.0_merged.bin
├── README.md
└── CHANGELOG.md
```

---

## ⚠️ Notes

- Firmware features depend on the connected hardware. For example, SD-card functions require compatible SD-card hardware.
- Wi-Fi features that retrieve online information require a working network connection.
- The firmware binaries in the supplied release package are prebuilt images from the provided archives. The source version label and the version displayed by a prebuilt binary may not match if the binary was not rebuilt from that source.
- Hardware testing should be performed on the target device before describing this release as fully tested.

---

## 📄 License

See the repository's license file, if present.
