# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP-IDF (v5.5.2) firmware for ESP32-family microcontrollers. The device controls an electric water heater based on solar panel light intensity. Supported boards: ESP32 (Olimex ESP32-EVB), ESP32-S2 (Wemos S2 Mini), ESP32-S3 (Waveshare ESP32-S3-ETH).

## Build Commands

```bash
# Source ESP-IDF environment first (required in every shell session)
. $IDF_PATH/export.sh

# Set target (required once per board; clears sdkconfig)
idf.py set-target esp32        # or esp32s2, esp32s3

# Interactive configuration
idf.py menuconfig

# Build
idf.py build

# Flash and monitor (USB-A)
idf.py -p /dev/ttyUSB0 flash monitor

# Flash and monitor (USB-C)
idf.py -p /dev/ttyACM0 flash monitor

# Save current config as new defaults
idf.py save-defconfig
```

Board-specific debug defaults: `sdkconfig.defaults.esp32.debug`, `sdkconfig.defaults.esp32s2.debug`, `sdkconfig.defaults.esp32s3.debug`. Copy the appropriate file over `sdkconfig.defaults` before setting the target.

## Architecture

### Manager + Registry Pattern

`main/mgr_ctrl.c` is the central hub. On startup it iterates `mgr_reg_list` (defined in `include/mgr_reg_list.h`) and calls each module's `init_fn`, then `run_fn` in sequence. Inter-module messages are dispatched via `send_fn`.

Each module exports exactly this interface (`mgr_reg_t` in `include/mgr_reg.h`):
- `init_fn` — allocate resources, start FreeRTOS task
- `done_fn` — tear down
- `run_fn` — called once by manager after all inits complete
- `send_fn` — receive a `msg_t` from the manager
- `get_fn` — optional bulk-read callback for producers (e.g. WiFi scan list consumed by both LCD and MQTT without those modules calling each other)

**Registry ordering matters:** `eth_ctrl` must be the first entry; `mqtt_ctrl` must be the last.

### Module System

All modules live under `modules/`. Each is an ESP-IDF component with its own `CMakeLists.txt` and `Kconfig` file. Modules are conditionally compiled based on `CONFIG_<NAME>_CTRL_ENABLE` — set via menuconfig or `sdkconfig.defaults`.

`main/CMakeLists.txt` conditionally appends each enabled module to `INCLUDE_LIST` and `PRIV_REQUIRE_LIST`. It also declares all modules unconditionally under `CMAKE_BUILD_EARLY_EXPANSION` to keep the component dependency graph consistent before sdkconfig exists.

`include/mgr_reg_list.h` is the registry — it `#include`s each enabled module's header and statically initializes the `mgr_reg_list[]` array. When adding a new module, register it here.

### Communication Flow

Each module runs its own FreeRTOS task with an internal message queue (`QueueHandle_t`). The manager routes `msg_t` messages between modules via `send_fn`. `mqtt_ctrl` is the bridge to the external MQTT broker — it receives outbound messages from other modules and forwards inbound MQTT payloads (JSON) to the appropriate module by topic.

### Custom Drivers

`drivers/tsl2561/` — TSL2561 light intensity sensor. Listed as an `EXTRA_COMPONENT_DIRS` in root `CMakeLists.txt`, alongside `modules/`.

### LCD Stack

`modules/lcd_ctrl/` drives an ILI9341V display with NS2009 touch. The UI is built with LVGL (managed component `lvgl__lvgl`). Display and touch drivers are in `ili9341v.c` and `ns2009.c`; see `docs/LCD.md` for wiring and calibration details.

### New Module Reference

Use `modules/template_ctrl/` as the implementation template. The pattern: static `QueueHandle_t` + `TaskHandle_t` + `SemaphoreHandle_t`, one FreeRTOS task, JSON parsing via cJSON, and the five exported `*Ctrl_*` functions registered in `mgr_reg_list.h`.

## Key Documentation

- `docs/mqtt.md` — MQTT topic structure and JSON message formats
- `docs/build.md` — Board-specific flash/serial connection notes
- `docs/LCD.md` — LCD wiring, touch calibration, LVGL notes
- `docs/memory.md` — Heap profiling; use `scripts/parse_mem_log.py` to analyze logs
