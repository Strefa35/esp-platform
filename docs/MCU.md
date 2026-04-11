<!-- markdownlint-disable-file MD013 MD060 -->

# MCU and target comparison

This document complements [BOARD.md](BOARD.md) with a broader comparison of **ESP-IDF targets** relevant to this repository.
It includes the currently used targets (`esp32`, `esp32s2`, `esp32s3`) plus nearby market options that are commonly visible on **Amazon.com** (`esp32c2`, `esp32c3`, `esp32c5`, `esp32c6`, `esp32h2`, `esp32p4`).

> Market visibility notes below are a quick snapshot from **April 2026** and should be treated as a buying guide, not a permanent stock guarantee.

---

## Board comparison (summary tables)

### IDF target family summary

| IDF target | CPU / architecture | Native wireless | Main strengths | Main trade-offs | Typical board examples |
| ---------- | ------------------ | --------------- | -------------- | --------------- | ---------------------- |
| `esp32` | Dual-core Xtensa LX6, up to 240 MHz | 2.4 GHz Wi‑Fi 4 + Bluetooth Classic/LE | Very mature ecosystem, good Ethernet options, broad community support | Oldest platform in the set, no Wi‑Fi 6 | Olimex ESP32-EVB, ESP32-DevKitC |
| `esp32s2` | Single-core Xtensa LX7, up to 240 MHz | 2.4 GHz Wi‑Fi 4 | Native USB, simple Wi‑Fi-only designs, low cost | No Bluetooth | LOLIN / Wemos S2 Mini |
| `esp32s3` | Dual-core Xtensa LX7, up to 240 MHz | 2.4 GHz Wi‑Fi 4 + BLE 5 | Best mainstream ESP32 choice for LVGL, PSRAM, USB, camera/AI-lite | Still 2.4 GHz only | Waveshare ESP32-S3-ETH, ESP32-S3-DevKitC, XIAO ESP32S3 |
| `esp32c2` | Single-core RISC-V | 2.4 GHz Wi‑Fi 4 + BLE 5 | Very low-cost entry point, compact IoT nodes | Lower performance and fewer premium board options | ESP8684 / ESP32-C2 mini boards |
| `esp32c3` | Single-core RISC-V, up to 160 MHz | 2.4 GHz Wi‑Fi 4 + BLE 5 | Excellent low-power general-purpose IoT target, strong security, very common on the market | Single-core only | XIAO ESP32C3, ESP32-C3-DevKitM-1, SuperMini C3 |
| `esp32c5` | Single-core RISC-V, up to 240 MHz | **Dual-band** Wi‑Fi 6 (2.4/5 GHz) + BLE 5 + 802.15.4 | Best radio feature set for future-facing wireless products, less congestion on 5 GHz | Newer platform, smaller software and board ecosystem than ESP32/C3/S3 | ESP32-C5-DevKitC-1, Waveshare ESP32-C5 boards |
| `esp32c6` | Single-core RISC-V, up to 160 MHz + LP core | 2.4 GHz Wi‑Fi 6 + BLE 5 + 802.15.4 | Strong choice for Matter / Thread / Zigbee / smart-home gateways and nodes | Less raw compute than S3/P4 for GUI-heavy work | ESP32-C6-DevKitC-1, ESP32-C6-DevKitM-1, XIAO ESP32C6 |
| `esp32h2` | Single-core RISC-V, low-power focus | BLE 5 + 802.15.4 (no Wi‑Fi) | Ideal Thread/Zigbee endpoint or companion radio | No Wi‑Fi, not a standalone Wi‑Fi controller target | ESP32-H2-DevKitM-1, Waveshare ESP32-H2 mini |
| `esp32p4` | Dual-core RISC-V up to 400 MHz + LP core | **No native radio** on the SoC itself | Best option for rich HMI, camera, audio, Ethernet, edge compute, large I/O | Usually needs a companion radio chip such as `ESP32-C6` for wireless | ESP32-P4-Function-EV-Board, Waveshare ESP32-P4-WIFI6 |

### Repo support status

| Target | Status in this repository | Notes |
| ------ | ------------------------- | ----- |
| `esp32` | **Supported now** | Debug defaults available in `sdkconfig.defaults.esp32.debug` |
| `esp32s2` | **Supported now** | Debug defaults available in `sdkconfig.defaults.esp32s2.debug` |
| `esp32s3` | **Supported now** | Debug defaults available in `sdkconfig.defaults.esp32s3.debug` |
| `esp32c2` | Candidate | Good for low-cost Wi‑Fi/BLE builds |
| `esp32c3` | Candidate | Strong low-power and low-cost upgrade path |
| `esp32c5` | Candidate | Attractive if 5 GHz Wi‑Fi 6 matters |
| `esp32c6` | Candidate | Best candidate for Matter / Thread / Zigbee work |
| `esp32h2` | Companion / specialized candidate | Best used where Wi‑Fi is not required |
| `esp32p4` | Future high-end candidate | Best for display-heavy, camera, HMI, and Ethernet-centric builds |

### Amazon.com market snapshot (April 2026)

| Target | Quick visibility on Amazon.com | Example listings seen in search |
| ------ | ------------------------------ | ------------------------------- |
| `esp32c3` | **Very high** — search showed **over 8,000 results** | Seeed XIAO ESP32C3, ESP32-C3-DevKitM-1, SuperMini C3 boards |
| `esp32c2` | **Moderate** — search showed **468 results** | Generic ESP32-C2 / ESP8684 mini boards |
| `esp32c6` | **Moderate and growing** — search showed **541 results** | ESP32-C6-DevKitC-1-N8, ESP32-C6-DevKitM-1-N4, XIAO ESP32C6 |
| `esp32h2` | **Moderate** — search showed **375 results** | ESP32-H2-DevKitM-1, Waveshare ESP32-H2 mini |
| `esp32c5` | **Moderate** — search showed **414 results** | ESP32-C5 DevKit boards, Waveshare ESP32-C5 |
| `esp32p4` | **Moderate and niche** — search showed **407 results** | Waveshare ESP32-P4-WIFI6, ESP32-P4 touch / HMI dev boards |

---

### Cross-platform quick comparison

This table follows the same **board-oriented comparison style** as [BOARD.md](BOARD.md).
For the currently supported targets, it uses the exact reference boards from this repository; for the remaining targets, it shows representative development boards commonly found on the market.

| Feature | `esp32` | `esp32s2` | `esp32s3` | `esp32c2` | `esp32c3` | `esp32c5` | `esp32c6` | `esp32h2` | `esp32p4` |
| ------- | ------- | --------- | --------- | --------- | --------- | --------- | --------- | --------- | --------- |
| Reference board / example | Olimex ESP32-EVB | Wemos / LOLIN ESP32-S2 Mini | Waveshare ESP32-S3-ETH | ESP32-C2 / ESP8684 mini board | XIAO ESP32C3 / ESP32-C3-DevKitM-1 | ESP32-C5-DevKitC-1 / Waveshare ESP32-C5 | ESP32-C6-DevKitC-1 / XIAO ESP32C6 | ESP32-H2-DevKitM-1 | ESP32-P4-Function-EV-Board / Waveshare ESP32-P4-WIFI6 |
| CPU | Dual Xtensa LX6, up to 240 MHz | Single Xtensa LX7, up to 240 MHz | Dual Xtensa LX7, up to 240 MHz | Single RISC-V | Single RISC-V, up to 160 MHz | Single RISC-V, up to 240 MHz | Single RISC-V, up to 160 MHz + LP core | Single RISC-V, low-power focus | Dual RISC-V, up to 400 MHz + LP core |
| Typical flash | 4 MB | 4 MB | 16 MB | 2–4 MB | 4 MB | 4–16 MB | 4–8 MB | 2–4 MB | Board-dependent, often larger external flash |
| On-chip SRAM (typical) | ~520 KB | 320 KB + 16 KB RTC | 512 KB + 16 KB RTC | ~272 KB | 400 KB SRAM + 384 KB ROM | 384 KB SRAM | 512 KB SRAM | 320 KB SRAM + 4 KB LP memory | 768 KB SRAM + 8 KB TCM |
| Typical PSRAM | None on common WROOM boards; available on WROVER variants | 2 MB | 8 MB | None | Usually none | Optional / board-dependent | Optional / board-dependent | None | Optional / common on HMI boards |
| Onboard Ethernet | Yes, RMII PHY | No | Yes, SPI W5500 | No | Rare | Rare | Available on some boards | No | Common on advanced dev boards |
| Wi‑Fi | 2.4 GHz 802.11 b/g/n | 2.4 GHz 802.11 b/g/n | 2.4 GHz 802.11 b/g/n | 2.4 GHz Wi‑Fi 4 | 2.4 GHz Wi‑Fi 4 | **2.4 + 5 GHz Wi‑Fi 6** | **2.4 GHz Wi‑Fi 6** | No | No native Wi‑Fi |
| Bluetooth | Classic + LE | None | BLE 5 | BLE 5 | BLE 5 | BLE 5 | BLE 5 | BLE 5 | Via companion chip |
| Thread / Zigbee / 802.15.4 | No | No | No | No | No | **Yes** | **Yes** | **Yes** | Via companion chip |
| USB / serial to host | Micro‑USB via UART bridge | Native USB‑C | USB‑C | Usually USB-UART via USB‑C / Micro‑USB | USB‑C or USB-UART depending on board | USB‑C | USB‑C | USB‑C | USB‑C |
| Typical Linux serial device | `/dev/ttyUSB0` | `/dev/ttyACM0` | `/dev/ttyACM0` | Usually `/dev/ttyUSB0` | `/dev/ttyUSB0` or `/dev/ttyACM0` | Usually `/dev/ttyACM0` | Usually `/dev/ttyACM0` | Usually `/dev/ttyACM0` | Usually `/dev/ttyACM0` |
| LCD / LVGL fit | Medium | Low | **High** | Low | Low | Medium | Medium | Low | **Very high** |
| Best fit in this repo | Ethernet + LCD + relay controller | Minimal Wi‑Fi / sensor bring-up | Ethernet + high-memory S3 builds | Budget Wi‑Fi/BLE node | Low-power general IoT node | High-end wireless node | Matter / Thread / Zigbee node | Thread / Zigbee endpoint | HMI / camera / edge host |
| Repo fit today | Supported | Supported | Supported | Candidate | Candidate | Candidate | Candidate | Specialized | Future high-end |

---

## Practical selection guide

| If you want… | Best target(s) |
| ------------ | -------------- |
| Lowest-cost Wi‑Fi + BLE node | `esp32c2`, `esp32c3` |
| Mature and easy bring-up | `esp32`, `esp32s3` |
| Native USB with Wi‑Fi only | `esp32s2` |
| Best current all-rounder for displays and memory-heavy firmware | `esp32s3` |
| Matter / Thread / Zigbee smart-home work | `esp32c6`, `esp32h2` |
| Dual-band Wi‑Fi 6 and future-proof radio stack | `esp32c5` |
| Rich GUI, camera, audio, Ethernet, HMI | `esp32p4` + wireless companion if needed |

## Notes

- `esp32p4` is **not** a drop-in wireless replacement for the classic ESP32 line; it is a higher-performance application processor and typically pairs with a wireless companion such as `ESP32-C6`.
- `esp32h2` is best viewed as a **Thread/Zigbee/BLE specialist**, not a general Wi‑Fi MCU.
- For this repository, the most realistic next additions after the currently supported targets are usually **`esp32c3`**, **`esp32c6`**, and **`esp32p4`**, depending on whether the priority is cost, smart-home radio protocols, or HMI performance.
