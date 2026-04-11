/**
 * @file ili9341v.h
 * @author A.Czerwinski@pistacje.net
 * @brief TFT LCD Single Chip Driver
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __ILI9341V_H__
#define __ILI9341V_H__


#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "lcd_defs.h"

/** Called from the SPI transfer-done ISR/context when a flush completes; @p ctx is user-defined. */
typedef void (*lcd_flush_done_cb_t)(void* ctx);

/**
 * @brief Initialize SPI, panel, backlight, and allocate DMA frame buffers into @p lcd_ptr.
 *
 * @param lcd_ptr Filled with resolution, buffer size, and buffer pointers.
 * @return ESP_OK on success, or an ESP-IDF driver error.
 */
esp_err_t lcd_InitDisplayHw(lcd_t* lcd_ptr);

/**
 * @brief Tear down panel, SPI bus, and backlight resources.
 *
 * @return ESP_OK or the error from `spi_bus_free` / panel delete.
 */
esp_err_t lcd_DoneDisplayHw(void);

/**
 * @brief Push an RGB565 rectangle to the panel (inclusive pixel range).
 *
 * @param x1 Left column.
 * @param y1 Top row.
 * @param x2 Right column.
 * @param y2 Bottom row.
 * @param color_map Pixel data in RGB565, row-major.
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if the panel is not initialized.
 */
esp_err_t lcd_FlushDisplayArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const void* color_map);

/**
 * @brief Register a callback invoked after each asynchronous flush completes.
 *
 * @param cb   Called from the panel I/O “color transfer done” path; may be NULL.
 * @param ctx  Opaque pointer passed to @p cb.
 * @return ESP_OK (always).
 */
esp_err_t lcd_SetFlushDoneCallback(lcd_flush_done_cb_t cb, void* ctx);

/**
 * @brief Set backlight brightness via LEDC (0 = off, 100 = full).
 *
 * @param percent Brightness 0–100.
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if hardware init did not run.
 */
esp_err_t lcd_SetBacklightPercent(uint8_t percent);

#endif /* __ILI9341V_H__ */
