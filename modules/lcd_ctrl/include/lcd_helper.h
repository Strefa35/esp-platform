/**
 * @file lcd_helper.h
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Helper
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __LCD_HELPER_H__
#define __LCD_HELPER_H__

#include <stdio.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t lcd_InitHelper(void);
esp_err_t lcd_DoneHelper(void);

void lcd_UpdateUid(const char* uid);
void lcd_UpdateMac(const uint8_t mac[6]);
void lcd_UpdateIp(uint32_t ip_addr);


#endif /* __LCD_HELPER_H__ */
