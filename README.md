# SNL GAMING — v10.0

Release package prepared from the two supplied source archives.

## Choose the correct firmware

- **Standard ESP32:** `firmware/SNL_GAMING_ESP32_v10.0_merged.bin`
- **ESP32 OLED wiring variant:** `firmware/SNL_GAMING_ESP32_OLED_v10.0_merged.bin`

The `*_merged.bin` files are complete 4 MB flash images. They include the bootloader,
partition table, boot_app0 and application image, based on the supplied Arduino build
metadata (`FlashSize=4M`, `PartitionScheme=huge_app`).

The `*_app-only.bin` files contain only the application. Use them only when you
intentionally want to update the application partition and already have the matching
bootloader/partition layout installed. For normal first-time flashing, use the
`*_merged.bin` file.

## Flashing

Use a trusted ESP32 flashing utility that supports writing a merged/full-flash image
at address `0x0000`. Select the merged file for your board, connect the ESP32 over USB,
select the correct serial port, and flash at address `0x0000`. Do not flash the OLED
image onto the standard wiring variant or vice versa. Back up anything important
before flashing; flashing may erase settings stored in flash.

## Source layout

- `source/ESP32/` — standard ESP32 source; OLED uses SDA 21 / SCL 22.
- `source/ESP32_OLED/` — alternate OLED wiring source; SDA 5 / SCL 4.

## Notes

- The visible firmware version strings in both `.ino` files were changed from v7.0
  to v10.0.
- The binary files are the prebuilt outputs supplied in the archives; they were not
  rebuilt after changing the source version strings. Therefore the source tree says
  v10.0, but the prebuilt firmware's on-device version text may still show v7.0.
- No hardware test was performed.
