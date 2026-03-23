# LCD Controller Module

## Overview
`lcd_ctrl` provides LCD and touch support for the ESP platform on **ESP32 target only**. The module integrates:
- ILI9341 display controller over SPI
- NS2009 touch controller over I2C
- LVGL 9.x as GUI framework

Current implementation target:
- Board family: ESP32 + MOD-LCD2.8RTP class hardware
- Logical resolution: 320x240


## Target Restriction
The module is intentionally limited to `IDF_TARGET=esp32`:
- Kconfig: `LCD_CTRL_ENABLE` depends on `IDF_TARGET_ESP32`
- Build-time guard: `#error` in `lcd_ctrl.c` for non-ESP32 builds


## Component Structure

### `lcd_ctrl.c`
Top-level controller entry points and message dispatch:
- `LcdCtrl_Init()`: initializes helper/UI/hardware stack and starts LCD task
- `LcdCtrl_Run()`: reserved runtime hook
- `LcdCtrl_Done()`: stops task and releases resources
- `LcdCtrl_Send()`: enqueues messages to LCD internal queue

Handled message types:
- `MSG_TYPE_INIT`, `MSG_TYPE_RUN`, `MSG_TYPE_DONE`
- `MSG_TYPE_MGR_UID` -> updates UID on UI
- `MSG_TYPE_ETH_MAC` -> updates MAC on UI
- `MSG_TYPE_ETH_IP` -> updates IP on UI


### `lcd_hw.c`
Hardware aggregation layer:
- initializes display driver (`lcd_InitDisplayHw`)
- initializes touch driver (`ns2009_Init`) using active LCD resolution
- deinitializes touch/display in reverse order


### `ili9341v.c`
ILI9341 display backend:
- SPI bus and panel IO setup via ESP-IDF `esp_lcd`
- panel init through `esp_lcd_ili9341`
- orientation setup: swap XY enabled, mirror disabled
- async flush completion callback for LVGL
- DMA-capable double buffers allocated with `spi_bus_dma_memory_alloc`

Fixed hardware mapping in current implementation:
- SPI host: `SPI2_HOST`
- Display resolution: `320x240`
- Backlight GPIO: `4`
- SPI pins: `SCLK=14`, `MOSI=2`, `DC=15`, `CS=17`


### `ns2009.c`
NS2009 touch backend:
- I2C master bus/device init and probe
- register reads for `X`, `Y`, and `Z1`
- two-sample averaging for stable raw data
- calibration mapping from raw range to display coordinates
- optional pressure threshold logic
- returns `ESP_ERR_NOT_FOUND` when touch is not valid


### `lcd_helper.c`
LVGL and UI integration:
- `lv_init()`, display creation, RGB565 buffer configuration
- LVGL flush callback using `lcd_FlushDisplayArea()`
- touch input callback reading NS2009 state
- LVGL tick timer (2 ms period)
- dedicated LVGL task calling `lv_timer_handler()` every 10 ms

Runtime UI data model includes:
- UID
- MAC
- IPv4 address

Data is updated from:
- incoming controller messages (`lcd_UpdateUid`, `lcd_UpdateMac`, `lcd_UpdateIp`)
- periodic ETH netif read for fallback IP refresh (`ETH_DEF`)


## UI Description
Current UI is a 3-page layout with bottom navigation:
- **Time**: local time (`HH:MM:SS`)
- **Config**: timezone and default NTP server
- **Device**: UID, MAC, IP and enabled controller list

Refresh behavior:
- UI data refresh timer runs every 1000 ms
- Controllers list is composed at runtime from enabled `CONFIG_*_CTRL_ENABLE`


## Kconfig Options
Main options available in `modules/lcd_ctrl/Kconfig.inc`:
- `LCD_CTRL_ENABLE`
- log levels for `lcd_ctrl`, `lcd_helper`, `lcd_hw`, `ili9341v`, `ns2009`
- NS2009 calibration and behavior:
  - I2C port/SDA/SCL/frequency
  - touch threshold and optional pressure requirement
  - raw min/max for X and Y
  - axis swap and inversion flags
- UI rotation choice:
  - 0 / 90 / 180 / 270 degrees


## Dependencies
Module dependencies:
- `lvgl/lvgl` (9.x)
- `esp_lcd_ili9341`
- ESP-IDF components: `driver`, `esp_lcd`, `esp_netif`, `esp_timer`, `lvgl`


## Notes
- This module currently assumes fixed display pin mapping from `ili9341v.c`.
- Timezone is read from runtime `TZ`; if missing, it falls back to SYS controller Kconfig values.
- Displayed NTP server comes from `CONFIG_SYS_CTRL_NTP_SERVER_DEFAULT` when available.
