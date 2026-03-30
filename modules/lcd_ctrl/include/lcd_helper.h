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
#include <stdint.h>

#include "esp_err.h"

#define LCD_MASK_ETH_CONNECTED     (1 << 0)
#define LCD_MASK_WIFI_CONNECTED    (1 << 1)
#define LCD_MASK_MQTT_CONNECTED    (1 << 2)
#define LCD_MASK_BT_CONNECTED      (1 << 3)
#define LCD_MASK_AMBIENT_LUX       (1 << 4)
#define LCD_MASK_AMBIENT_THRESHOLD (1 << 5)
#define LCD_MASK_UID               (1 << 6)
#define LCD_MASK_MAC               (1 << 7)
#define LCD_MASK_IP                (1 << 8)

/**
 * Payload for lcd_UpdateData(): OR together LCD_MASK_* bits.
 * - Connection: d_bool[0]=ETH, [1]=WIFI, [2]=MQTT, [3]=BT.
 * - Ambient: d_uint32[0]=lux (LCD_MASK_AMBIENT_LUX), d_uint32[1]=threshold (LCD_MASK_AMBIENT_THRESHOLD).
 * - UID: uid[] null-terminated (LCD_MASK_UID).
 * - MAC: u.d_uint8[0..5] (LCD_MASK_MAC).
 * - IP: ip IPv4 network byte order (LCD_MASK_IP); string updated only when ip != 0.
 */
typedef struct {
  union {
    bool      d_bool[8];
    uint8_t   d_uint8[8];
    uint16_t  d_uint16[4];
    uint32_t  d_uint32[2];
    uint64_t  d_uint64[1];
  } u;
  char     uid[32];
  uint32_t ip;
} lcd_update_t;


esp_err_t lcd_InitHelper(void);
esp_err_t lcd_DoneHelper(void);

void lcd_UpdateData(const uint32_t mask, const lcd_update_t* update);

#endif /* __LCD_HELPER_H__ */
