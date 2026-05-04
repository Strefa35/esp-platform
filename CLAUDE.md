# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP-IDF (v5.5.2) firmware for ESP32-family microcontrollers. The device controls an electric water heater based on solar panel light intensity. Supported boards: ESP32 (Olimex ESP32-EVB), ESP32-S2 (Wemos S2 Mini), ESP32-S3 (Waveshare ESP32-S3-ETH).

## Build Commands

```bash
# Source ESP-IDF environment first (required in every shell session)
. $IDF_PATH/export.sh

# Set target (required once per board; clears sdkconfig)
cp sdkconfig.defaults.esp32.debug sdkconfig.defaults
idf.py set-target esp32        # or esp32s2, esp32s3

# Interactive configuration
idf.py menuconfig

# Build
idf.py build

# Flash and monitor (ESP32-EVB: USB-A → ttyUSB0; S2/S3: USB-C → ttyACM0)
idf.py -p /dev/ttyUSB0 flash monitor
idf.py -p /dev/ttyACM0 flash monitor

# Save current config as new defaults
idf.py save-defconfig
```

Board-specific debug defaults: `sdkconfig.defaults.esp32.debug`, `sdkconfig.defaults.esp32s2.debug`, `sdkconfig.defaults.esp32s3.debug`. Copy the appropriate file over `sdkconfig.defaults` before `idf.py set-target`.

The checked-in `sdkconfig.defaults` targets ESP32 with modules: `eth`, `wifi`, `relay`, `lcd`, `sys`, `sensor`, `cli`, `mqtt`.

## Architecture

### Manager + Registry Pattern

`main/mgr_ctrl.c` is the central hub. On startup it iterates `mgr_reg_list` (defined in `include/mgr_reg_list.h`) and calls each module's `init_fn`, then `run_fn` in sequence. Inter-module messages are dispatched via `send_fn`.

Each module exports exactly this interface (`mgr_reg_t` in `include/mgr_reg.h`):
- `init_fn` — allocate resources, start FreeRTOS task
- `done_fn` — tear down (called in **reverse** registry order)
- `run_fn` — called once by manager after all inits complete
- `send_fn` — receive a `msg_t` from the manager
- `get_fn` — optional bulk-read callback; consumers call `MGR_GetData(REG_*_CTRL, type, cb, ctx)` to pull snapshot data (e.g. Wi-Fi scan list) from a producer without those modules including each other

**Registry ordering matters:** `eth_ctrl` must be the first entry; `mqtt_ctrl` must be the last.

### Module System

All modules live under `modules/`. Each is an ESP-IDF component with its own `CMakeLists.txt` and `Kconfig` file. Modules are conditionally compiled based on `CONFIG_<NAME>_CTRL_ENABLE` — set via menuconfig or `sdkconfig.defaults`.

`main/CMakeLists.txt` conditionally appends each enabled module to `INCLUDE_LIST` and `PRIV_REQUIRE_LIST`. It also declares all modules unconditionally under `CMAKE_BUILD_EARLY_EXPANSION` to keep the component dependency graph consistent before sdkconfig exists.

`include/mgr_reg_list.h` is the registry — it `#include`s each enabled module's header and statically initializes the `mgr_reg_list[]` array. When adding a new module, register it here.

### Communication Flow

Each module runs its own FreeRTOS task with an internal message queue (`QueueHandle_t`). The manager routes `msg_t` messages between modules via `send_fn`. `mqtt_ctrl` is the bridge to the external MQTT broker — it receives outbound messages from other modules and forwards inbound MQTT payloads (JSON) to the appropriate module by topic.

`msg_t` fields (defined in `include/msg.h`):
- `type` — `msg_type_e` discriminant
- `from` / `to` — bitmasks of `REG_*_CTRL` flags (defined in `include/msg.h`); the manager fans out to all matching registry entries
- `payload` — union selected by `type`

If `msg.to` includes `REG_MGR_CTRL`, the manager also runs `mgr_ParseMsg` locally before forwarding (e.g. Ethernet IP → start MQTT, MQTT connected → subscribe topics).

### Task lifecycle and custom error codes (`err.h`)

Each module task loops on `xQueueReceive`. The `parseMsg` function returns one of three sentinel values from `include/err.h` to control the loop:

- `ESP_TASK_INIT` — reserved for `MSG_TYPE_INIT`
- `ESP_TASK_RUN` — reserved for `MSG_TYPE_RUN`
- `ESP_TASK_DONE` — `MSG_TYPE_DONE` received; task exits loop and calls `xSemaphoreGive`

The `done_fn` sends `MSG_TYPE_DONE` directly to the module's own queue (bypassing the manager), then blocks on `xSemaphoreTake` until the task exits. This is the universal teardown pattern used by every module.

### MQTT Topic Routing

Inbound MQTT payloads of the form `{uid}/req/{module-name}` are routed by the manager to the matching module's `send_fn` by name. `mqtt_ctrl` carries `REG_INT_CTRL` in its `type` field, which marks it as internal-only and prevents external addressing.

The device UID is derived from the last 3 bytes of the Ethernet MAC: `ESP/XXXXXX` (e.g. `ESP/12AB34`). It is broadcast to all modules via `MSG_TYPE_MGR_UID` during initialization so each module can construct its own MQTT topics.

### LCD Update Path

Modules outside `lcd_ctrl` push display state via two mechanisms:

1. **Direct call** (within `lcd_ctrl` task context or thread-safe helper): `lcd_UpdateData(mask, &update)` defined in `modules/lcd_ctrl/include/lcd_helper.h`.
2. **Message bus** (from any other module): send `MSG_TYPE_LCD_DATA` with a `payload_lcd_t` (`mask` + `d_uint32[8]` array). The manager forwards it to `lcd_ctrl`, which calls `lcd_UpdateData` internally.

All `LCD_MASK_*` bits are defined in `modules/lcd_ctrl/include/lcd_helper.h` — do not include it from outside `lcd_ctrl`. Non-LCD modules that need specific mask values (e.g. ambient lux) define local `#define` copies with a `/* Keep in sync with LCD_MASK_* in lcd_helper.h */` comment.

### Custom Drivers

`drivers/tsl2561/` — TSL2561 light intensity sensor. Listed as an `EXTRA_COMPONENT_DIRS` in root `CMakeLists.txt`, alongside `modules/`.

### LCD Stack

`modules/lcd_ctrl/` drives an ILI9341V display (320×240) with NS2009 touch over I2C. The UI is built with LVGL 9.x (managed component `lvgl__lvgl`). **`lcd_ctrl` supports only ESP32 and ESP32-S3 targets** — a compile-time guard in `lcd_ctrl.c` enforces this.

Internal layers:
- `lcd_hw.c` — hardware composition (display + touch init/deinit)
- `ili9341v.c` — ILI9341 SPI backend with LVGL flush callback
- `ns2009.c` — touch backend with coordinate calibration and pressure filtering
- `lcd_helper.c` — LVGL init, display/touch binding, software rotation in flush callback, periodic tick task, `lcd_UpdateData`
- `lcd_ctrl.c` — module entry points and message handling

Display rotation is handled in the **flush callback** (rotating pixel data with `lv_draw_sw_rotate`). Do **not** manually unrotate touch coordinates in the NS2009 read callback — LVGL handles that automatically after `lv_indev_set_display`.

Material Icons font: `modules/lcd_ctrl/fonts/lv_font_material_icons_22.c`. Regenerate with `modules/lcd_ctrl/fonts/gen_material_icons_font.sh` (requires Docker + `lv_font_conv`).

See `docs/LCD_CTRL.md` for wiring, calibration details, and current UI structure.

### New Module Reference

Use `modules/template_ctrl/` as the implementation template. The pattern: static `QueueHandle_t` + `TaskHandle_t` + `SemaphoreHandle_t`, one FreeRTOS task, JSON parsing via cJSON, and four exported `XxxCtrl_*` functions (`Init`, `Done`, `Run`, `Send`) registered in `mgr_reg_list.h`.

Assign the new module a unique `REG_*_CTRL` bit in `include/msg.h` and a `MSG_TYPE_*` variant if it introduces new message types. Also add a `if(CONFIG_<NAME>_CTRL_ENABLE)` block to `main/CMakeLists.txt` and an `orsource` line to `modules/Kconfig.inc`.

`cli_ctrl` demonstrates splitting CLI sub-commands across multiple files (`cli_lcd.c`, `cli_wifi.c`) — each file registers its own `esp_console` commands and is conditionally compiled on the combination of `CONFIG_CLI_CTRL_ENABLE && CONFIG_<OTHER>_CTRL_ENABLE`.

## Key Documentation

### Architecture & Design

- `docs/ARCHITECTURE.md` — comprehensive architecture reference with sequence and flow diagrams

### Build & Hardware

- `docs/BUILD.md` — board-specific build, flash, and serial connection notes
- `docs/BOARD.md` — hardware comparison table for all three supported boards (GPIO pinouts, flash/PSRAM, Ethernet type)
- `docs/MCU.md` — SoC-level details and memory map notes

### Module Reference

Each module has a dedicated doc at `docs/<MODULE>_CTRL.md`:

- `docs/ETH_CTRL.md` — Ethernet: MAC/PHY init, link-state events
- `docs/WIFI_CTRL.md` — Wi-Fi STA: scan, connect, IP events, `get_fn` scan results
- `docs/MQTT_CTRL.md` — MQTT bridge: topic routing, NVS config, broker lifecycle
- `docs/RELAY_CTRL.md` — relay GPIO control via MQTT set/get
- `docs/SENSOR_CTRL.md` — TSL2561 ambient-light sensor, per-sensor tasks
- `docs/LCD_CTRL.md` — LCD wiring, touch calibration, LVGL UI structure, Kconfig options
- `docs/SYS_CTRL.md` — NTP sync, timezone management, system info MQTT
- `docs/CLI_CTRL.md` — UART/USB-CDC REPL, sub-command split pattern
- `docs/CFG_CTRL.md` — configuration controller placeholder
- `docs/GPIO_CTRL.md` — GPIO controller placeholder
- `docs/COAP_CTRL.md` — CoAP/UDP peer-to-peer module (in progress)
- `docs/TEMPLATE_CTRL.md` — reference implementation / new-module scaffold

### Tools & Scripts

- `docs/MEMORY.md` — heap profiling workflow (flash size + runtime heap)
- `docs/PARSE_MEM_LOG.md` — `scripts/parse_mem_log.py` usage and output format
- `docs/WIFI.md` — Wi-Fi provisioning and connection flow (extended reference)
