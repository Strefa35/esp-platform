<!-- markdownlint-disable-file MD060 -->

# Supported hardware

This firmware is built with **ESP-IDF 5.5.x** and targets three **board families** used in this repository. Each family maps to one IDF target (`esp32`, `esp32s2`, `esp32s3`). Vendor pages and Espressif hardware references are linked from [README.md](../README.md).

Board-specific Kconfig defaults live next to the project root:

| IDF target | Use as `sdkconfig.defaults` (copy before `idf.py set-target …`) |
| ---------- | --------------------------------------------------------------- |
| `esp32`    | `sdkconfig.defaults.esp32.debug`                              |
| `esp32s2`  | `sdkconfig.defaults.esp32s2.debug`                            |
| `esp32s3`  | `sdkconfig.defaults.esp32s3.debug`                            |

Flash size, partition table, and optional modules must match your **actual** module; see [README.md](../README.md) (ESP32 custom partition table) and [build.md](build.md) (serial and build notes).

---

## Board comparison (summary tables)

SRAM/PSRAM figures follow vendor or Espressif documentation; usable heap depends on Wi‑Fi stack, Bluetooth, LVGL, and other enabled features.

### Cross-platform quick comparison

This table is intended for **at-a-glance differences** across the three supported board families.

| Feature | Olimex ESP32-EVB | Wemos / LOLIN ESP32-S2 Mini | Waveshare ESP32-S3-ETH |
| ------- | ---------------- | --------------------------- | ---------------------- |
| IDF target | `esp32` | `esp32s2` | `esp32s3` |
| CPU | Dual Xtensa LX6, up to 240 MHz | Single Xtensa LX7, up to 240 MHz | Dual Xtensa LX7, up to 240 MHz |
| Typical flash | 4 MB | 4 MB | 16 MB |
| On-chip SRAM (typical) | ~520 KB | 320 KB + 16 KB RTC | 512 KB + 16 KB RTC |
| Typical PSRAM | None | 2 MB | 8 MB |
| Onboard Ethernet | Yes, RMII PHY (LAN8710A/LAN87xx) | No | Yes, SPI Ethernet (W5500) |
| Wi‑Fi | 2.4 GHz 802.11 b/g/n | 2.4 GHz 802.11 b/g/n | 2.4 GHz 802.11 b/g/n |
| Bluetooth | Classic + LE | None | LE |
| USB / serial to host | Micro‑USB via UART bridge | Native USB‑C | USB‑C |
| Typical Linux serial device | `/dev/ttyUSB0` | `/dev/ttyACM0` | `/dev/ttyACM0` |
| LCD enabled in checked-in debug defaults | Yes | No | No |
| Best fit in this repo | Ethernet + LCD + relay-centric controller | Minimal Wi‑Fi / sensor bring-up | Ethernet + high-memory S3 builds |
| Reference defaults file | `sdkconfig.defaults.esp32.debug` | `sdkconfig.defaults.esp32s2.debug` | `sdkconfig.defaults.esp32s3.debug` |

### Olimex ESP32-EVB

Industrial-style IoT board with **Ethernet (RMII)**, Wi‑Fi, relays, SD card, and more. This repository assumes the **Olimex** layout for Ethernet PHY options.

| Parameter                    | Value                                                                 |
| ---------------------------- | --------------------------------------------------------------------- |
| IDF target                   | `esp32`                                                               |
| Typical module               | ESP32-WROOM-32E or ESP32-WROOM-32UE (‑N4, 4 MB flash, no PSRAM)         |
| CPU                          | Dual Xtensa LX6, up to 240 MHz                                        |
| External flash (typical)     | 4 MB                                                                  |
| PSRAM                        | None on standard ‑N4 EVB module                                       |
| On-chip SRAM (typical)       | ~520 KB total (ESP32)                                                 |
| Ethernet                     | Onboard LAN8710A PHY, RMII, internal MAC                              |
| Wi‑Fi                        | 802.11 b/g/n 2.4 GHz                                                  |
| Bluetooth                    | Classic + LE                                                          |
| USB to host                  | UART bridge (e.g. CH340), Micro‑USB                                  |
| Typical serial device (Linux) | `/dev/ttyUSB0`                                                        |
| Reference defaults file      | `sdkconfig.defaults.esp32.debug`                                      |

#### Networking (ESP32-EVB defaults)

- Internal EMAC + **LAN87xx** PHY (`CONFIG_ETH_CTRL_PHY_LAN87XX=y`).
- PHY address **0**, PHY reset GPIO **disabled** (`-1`) in `sdkconfig.defaults.esp32.debug` — matches common EVB wiring.
- SMI defaults in Kconfig: **MDC GPIO 23**, **MDIO GPIO 18** (Espressif `eth_ctrl` defaults for ESP32).
- **Wi‑Fi** enabled in the same defaults file.

#### Flash and partitions

- Defaults select **4 MB** flash (`CONFIG_ESPTOOLPY_FLASHSIZE_4MB`) and a **custom** partition table: `config/partitions-esp32.csv` (factory app **1800K**). The layout must fit your module; do not exceed physical flash size.

#### Optional peripherals in this firmware

- **LCD**: ILI9341V + NS2009 touch (`CONFIG_LCD_CTRL_ENABLE=y` in ESP32 debug defaults). Wiring and calibration: [LCD.md](LCD.md).
- **Light sensor**: TSL2561 driver under `drivers/tsl2561/` (enabled in ESP32 debug defaults).
- Onboard **relays** and other EVB features are controlled when `relay_ctrl` and related options are enabled in Kconfig.

---

### Wemos / LOLIN ESP32-S2 Mini

Compact **ESP32-S2** board (USB‑C), pin‑compatible with many **D1 mini** shields.

| Parameter                    | Value                                          |
| ---------------------------- | ---------------------------------------------- |
| IDF target                   | `esp32s2`                                      |
| Typical SoC                  | ESP32-S2FN4R2 (4 MB flash, 2 MB PSRAM common) |
| CPU                          | Single Xtensa LX7, up to 240 MHz               |
| External flash (typical)     | 4 MB                                           |
| PSRAM                        | 2 MB (on ‑FN4R2)                               |
| On-chip SRAM (typical)       | 320 KB + 16 KB RTC (ESP32-S2)                  |
| Ethernet                     | Not present                                    |
| Wi‑Fi                        | 802.11 b/g/n 2.4 GHz                           |
| Bluetooth                    | None (ESP32-S2 has no Bluetooth)             |
| USB to host                  | Native USB‑C (CDC / JTAG)                      |
| Typical serial device (Linux) | `/dev/ttyACM0`                                |
| Reference defaults file      | `sdkconfig.defaults.esp32s2.debug`             |

#### Default profile in this repository

The checked-in `sdkconfig.defaults.esp32s2.debug` is **minimal**: **Ethernet**, **MQTT**, and **relay** controllers are **disabled**. It suits bring‑up or sensor‑only experiments; re‑enable modules in `idf.py menuconfig` if your hardware needs them.

#### Wi‑Fi and Ethernet (ESP32-S2 Mini)

- **No onboard Ethernet** — use Wi‑Fi only if you enable `wifi_ctrl`.

#### Flash configuration (ESP32-S2 Mini)

- The debug defaults file does **not** set flash size; configure **Serial flasher config → Flash size** to match your module (often **4 MB** on S2 Mini).

---

### Waveshare ESP32-S3-ETH

ESP32-S3 board with **W5500** 10/100 Ethernet on **SPI**, Wi‑Fi, Bluetooth LE; optional PoE/Camera variants per vendor.

| Parameter                    | Value                                          |
| ---------------------------- | ---------------------------------------------- |
| IDF target                   | `esp32s3`                                      |
| Typical SoC                  | ESP32-S3R8 (8 MB octal PSRAM, 16 MB flash typ.) |
| CPU                          | Dual Xtensa LX7, up to 240 MHz                 |
| External flash (typical)     | 16 MB (e.g. W25Q128 class)                    |
| PSRAM                        | 8 MB (octal, on ‑S3R8)                         |
| On-chip SRAM (typical)       | 512 KB + 16 KB RTC (ESP32-S3)                  |
| Ethernet                     | Onboard W5500, SPI to ESP32-S3                 |
| Wi‑Fi                        | 802.11 b/g/n 2.4 GHz                           |
| Bluetooth                    | Bluetooth 5 (LE)                               |
| USB to host                  | USB‑C                                          |
| Typical serial device (Linux) | `/dev/ttyACM0`                                |
| Reference defaults file      | `sdkconfig.defaults.esp32s3.debug`             |

#### Networking (ESP32-S3-ETH defaults)

- **SPI Ethernet** (`CONFIG_ETH_CTRL_USE_SPI_ETHERNET=y`), **W5500** (Kconfig `ETH_CTRL_USE_W5500` choice).
- GPIO assignment in `sdkconfig.defaults.esp32s3.debug` (Waveshare wiki alignment):

| Signal    | GPIO |
| --------- | ---- |
| SPI SCLK  | 13   |
| SPI MISO  | 12   |
| SPI CS0   | 14   |
| W5500 INT | 10   |
| W5500 RST | 9    |

- **Wi‑Fi** remains available on the S3 concurrently with SPI Ethernet (subject to RF and power design).

#### Flash and PSRAM

- Retail boards often ship with **16 MB flash** and **8 MB PSRAM**. The saved S3 debug defaults do **not** pin flash size; set **Flash size** (and PSRAM options if used) in `menuconfig` to match your board.

#### Optional peripherals (ESP32-S3-ETH)

- `CONFIG_RELAY_CTRL_ENABLE=n` in the checked-in S3 debug defaults — enable if you attach relays.
- **LCD** is not enabled in the S3 debug defaults; the LCD stack is documented for the ILI9341V + NS2009 setup in [LCD.md](LCD.md).

---

## Related documentation

| Topic                        | Document                                           |
| ---------------------------- | -------------------------------------------------- |
| Build, target switch, serial | [README.md](../README.md), [build.md](build.md) |
| LCD wiring and LVGL          | [LCD.md](LCD.md)                         |
| MQTT and JSON                | [mqtt.md](mqtt.md)                       |
| Heap logging                 | [memory.md](memory.md)                   |

If you use a different PCB revision, clone, or module flash size, treat the tables above as a baseline and verify **schematics**, **flash size**, and **menuconfig** against your hardware.
