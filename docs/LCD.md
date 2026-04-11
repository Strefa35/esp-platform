# LCD Module

## Overview

The `lcd_ctrl` module provides display and touch support for the ESP platform using:

- 2.8" LCD (`320x240`) with ILI9341 over SPI
- NS2009 touch panel over I2C
- LVGL 9.x as the GUI framework

The module is currently focused on **ESP32** and **ESP32-S3** targets (see `lcd_ctrl.c`); other SoCs fail the build guard.

Recent runtime changes worth knowing:

- LVGL display rotation is now handled in the **flush path** for partial-render mode, so non-zero `CONFIG_LCD_UI_ROTATION_*` settings affect the real panel output as expected.
- The touch input device is explicitly bound to the LVGL display, so LVGL rotates pointer coordinates internally.
- Status-bar icons now use fixed-size pressable slots, which makes resistive touch interaction more reliable than tapping raw labels.

## Current UI Status

### Main screen (`ui_main_screen`)

Current implementation includes:

- Top status bar with connectivity icons on the left: Ethernet, Wi-Fi, MQTT, Bluetooth
- Ethernet and Wi-Fi icons are placed in **button-like touch slots** with centered Material icons
- Settings button on the right side of the status bar
- Large digital clock and date in the left area
- Ambient light section (`Ambient (lx)`) with:
  - sun icon
  - current lux value text
  - threshold text
  - horizontal gradient track with value and threshold markers

Touch UX notes:

- Ethernet / Wi-Fi status actions are triggered on **`LV_EVENT_PRESSED`** instead of waiting for a full click/release cycle
- `LV_OBJ_FLAG_PRESS_LOCK` is used for pressable status icons to better tolerate resistive-touch jitter during release
- MQTT and Bluetooth icons currently remain status-only (non-clickable)

Important implementation note:

- The relay card block (water heater + circulation pump controls) exists in code, but is currently compiled out with `#if 0` in `ui_main_screen_create()`.

### Screensaver (`ui_screensaver`)

Implemented and active:

- On **cold boot** the active LVGL screen is the **screensaver** until the user completes a **stable touch** (then `lcd_switch_screen` opens the main UI).
- Analog clock (when `LV_USE_SCALE` is enabled), or digital fallback when disabled
- Date label
- Weather panel with icon, temperature, summary, and location
- Automatic return to the screensaver after idle timeout is **disabled** in code (`LCD_SCREENSAVER_IDLE_ENABLE` in `lcd_helper.c`); only the initial “wake from saver” path is active unless you re-enable it.

### Configuration screen

- Mockup exists (`docs/todo/lcd_mockup_config.png`)
- Dedicated UI screen implementation is not present yet

## Mockups (320x240)

Design reference files:

- `docs/todo/lcd_mockup_main.png`
- `docs/todo/lcd_mockup_config.png`
- `docs/todo/lcd_mockup_screensaver.png`

## Rotation and touch behavior

### Display rotation

`lv_display_set_rotation()` alone is not enough for this hardware when LVGL renders into partial draw buffers. The current implementation in `lcd_helper.c` now:

- checks `lv_display_get_rotation(display)` in the flush callback
- rotates the dirty rectangle with `lv_display_rotate_area()`
- rotates the RGB565 pixel data with `lv_draw_sw_rotate()`
- flushes the rotated area/buffer to `lcd_FlushDisplayArea()`

A scratch buffer (`s_rotated_buf`) is allocated only when the configured rotation is non-zero.

### Touch rotation and filtering

The NS2009 touch input is attached to the display with `lv_indev_set_display(s_touch_indev, s_display)`, which means LVGL applies the current display rotation during pointer processing. In other words: **do not manually “unrotate” touch coordinates in the read callback**.

The pressure filter still uses `CONFIG_LCD_NS2009_TOUCH_THRESHOLD`, but the effective runtime threshold is capped to **120** when `CONFIG_LCD_NS2009_REQUIRE_PRESSURE` is enabled. This prevents over-aggressive values from making valid taps disappear on resistive panels.

## Driver and Integration Layers

### `lcd_ctrl.c`

Controller-facing entry points and queue-based message handling. Handles `msg_t` types used for the status UI, including Ethernet and Wi-Fi link/IP events, MQTT events, and manager UID/MAC (see `lcdctrl_ParseMsg` in source).

### `lcd_helper.c`

LVGL integration layer:

- LVGL initialization
- display/touch binding
- software-assisted rotation in the flush callback when needed
- periodic LVGL tick and handler task
- UI update helpers used by LCD controller flow

### `lcd_hw.c`

Hardware composition layer for display + touch init/deinit.

### `ili9341v.c`

ILI9341 display backend:

- panel/bus setup via ESP-IDF `esp_lcd`
- LVGL flush callback integration
- DMA-capable frame buffer handling

### `ns2009.c`

Touch backend:

- I2C probe and reads
- coordinate conversion/calibration
- pressure / valid-touch filtering with a capped effective threshold

## Kconfig and Tuning Notes

Main LCD options are in `modules/lcd_ctrl/Kconfig.inc`, including:

- module enable/log levels
- touch calibration, axis swap/inversion, threshold options
- display orientation selection

Current checked-in ESP32 defaults enable:

- `CONFIG_LCD_CTRL_ENABLE=y`
- verbose LCD logging for helper / hardware / ILI9341 / NS2009 layers
- `CONFIG_LCD_NS2009_REQUIRE_PRESSURE=y`
- `CONFIG_LCD_NS2009_SWAP_XY=y`
- `CONFIG_LCD_NS2009_INVERT_X=y`
- `CONFIG_LCD_NS2009_INVERT_Y=y`

Font sizing for clock labels depends on enabled LVGL fonts in project config:

- e.g. `CONFIG_LV_FONT_MONTSERRAT_14`, `CONFIG_LV_FONT_MONTSERRAT_46`
- if a font is not enabled in config, related `lv_font_montserrat_*` symbol is not available

## Known Gaps / Next Work

- Implement dedicated configuration screen UI
- Re-enable and finalize the main screen relay panel
- Align docs/mockups with final runtime behavior as UI evolves
