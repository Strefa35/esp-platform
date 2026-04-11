/**
 * @file lcd_hw.h
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Hardware
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __LCD_HW_H__
#define __LCD_HW_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "lcd_defs.h"

/**
 * @brief Initialize display (ILI9341) and touch (NS2009) hardware for the given LCD context.
 *
 * @param lcd_ptr LCD context; resolution and buffers are filled by lower layers.
 * @return ESP_OK on success, or the first failing driver error.
 */
esp_err_t lcd_InitHw(lcd_t* lcd_ptr);

/**
 * @brief Release touch and display hardware (reverse order of init).
 *
 * @return ESP_OK if both subsystems shut down cleanly, or a subsystem error code.
 */
esp_err_t lcd_DoneHw(void);

#endif /* __LCD_HW_H__ */
