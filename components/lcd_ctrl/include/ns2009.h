/**
 * @file ns2009.h
 * @author A.Czerwinski@pistacje.net
 * @brief Touch Screen Controller Driver
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __NS2009_H__
#define __NS2009_H__


#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

#include "lcd_defs.h"

esp_err_t lcd_InitTouchHw(lcd_t* lcd_ptr);

#endif /* __NS2009_H__ */