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

#include "esp_err.h"

#include "lcd_defs.h"


esp_err_t lcd_InitDisplayHw(lcd_t* lcd_ptr);

#endif /* __ILI9341V_H__ */