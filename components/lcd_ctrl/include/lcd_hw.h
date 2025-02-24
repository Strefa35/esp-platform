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


esp_err_t lcd_InitHw(lcd_t* lcd_ptr);

#endif /* __LCD_HW_H__ */