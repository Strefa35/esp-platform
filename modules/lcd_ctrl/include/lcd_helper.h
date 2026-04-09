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
 * @brief Payload for lcd_UpdateData(): OR together `LCD_MASK_*` bits.
 *
 * - Connection: `d_bool[0]` ETH, `[1]` Wi-Fi, `[2]` MQTT, `[3]` BT.
 * - Ambient: `d_uint32[0]` lux (`LCD_MASK_AMBIENT_LUX`), `d_uint32[1]` threshold (`LCD_MASK_AMBIENT_THRESHOLD`).
 * - UID: `uid[]` null-terminated (`LCD_MASK_UID`).
 * - MAC: `u.d_uint8[0..5]` (`LCD_MASK_MAC`).
 * - IP: `ip` IPv4 network byte order (`LCD_MASK_IP`); runtime string updated only when `ip != 0`.
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

/**
 * @brief Bring up hardware, LVGL, timers, and the UI task (used by lcd_ctrl).
 *
 * @return ESP_OK on success, or an error from HW/LVGL setup.
 */
esp_err_t lcd_InitHelper(void);

/**
 * @brief Stop UI task, LVGL, timers, and release hardware (reverse of init).
 *
 * @return ESP_OK on success.
 */
esp_err_t lcd_DoneHelper(void);

/**
 * @brief Thread-safe merge of UI state from another task; see `lcd_update_t` and `LCD_MASK_*`.
 *
 * @param mask   Bitmask of fields to update.
 * @param update Pointer to new values; ignored if NULL.
 */
void lcd_UpdateData(const uint32_t mask, const lcd_update_t* update);

#endif /* __LCD_HELPER_H__ */
