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

typedef void(*lcd_flush_done_cb_t)(void* ctx);


esp_err_t lcd_InitDisplayHw(lcd_t* lcd_ptr);
esp_err_t lcd_DoneDisplayHw(void);
esp_err_t lcd_FlushDisplayArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const void* color_map);
esp_err_t lcd_SetFlushDoneCallback(lcd_flush_done_cb_t cb, void* ctx);
esp_err_t lcd_SetBacklightPercent(uint8_t percent);

#endif /* __ILI9341V_H__ */
