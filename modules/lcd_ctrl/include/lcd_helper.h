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

/* LCD_MASK_* bits for lcd_UpdateData() */
#define LCD_MASK_BOARD_MAC          (1 << 0)    // lcd_update_t.u.d_uint8[0..5]
#define LCD_MASK_BOARD_UID          (1 << 1)    // lcd_update_t.u.d_string[0]

#define LCD_MASK_ETH_CONNECTED      (1 << 4)    // lcd_update_t.u.d_bool[0]
#define LCD_MASK_WIFI_CONNECTED     (1 << 5)    // lcd_update_t.u.d_bool[0]
#define LCD_MASK_MQTT_CONNECTED     (1 << 6)    // lcd_update_t.u.d_bool[0]
#define LCD_MASK_BT_CONNECTED       (1 << 7)    // lcd_update_t.u.d_bool[0]

#define LCD_MASK_ETH_IP             (1 << 8)    // lcd_update_t.u.d_uint32[0]
#define LCD_MASK_ETH_NETMASK        (1 << 9)    // lcd_update_t.u.d_uint32[1]
#define LCD_MASK_ETH_GW             (1 << 10)   // lcd_update_t.u.d_uint32[2]
#define LCD_MASK_ETH_MAC            (1 << 11)   // lcd_update_t.u.d_uint8[0..5]

#define LCD_MASK_WIFI_IP            (1 << 12)   // lcd_update_t.u.d_uint32[0]
#define LCD_MASK_WIFI_NETMASK       (1 << 13)   // lcd_update_t.u.d_uint32[1]
#define LCD_MASK_WIFI_GW            (1 << 14)   // lcd_update_t.u.d_uint32[2]
#define LCD_MASK_WIFI_MAC           (1 << 15)   // lcd_update_t.u.d_uint8[0..5]

#define LCD_MASK_AMBIENT_LUX        (1 << 20)   // lcd_update_t.u.d_uint32[0]
#define LCD_MASK_AMBIENT_THRESHOLD  (1 << 21)   // lcd_update_t.u.d_uint32[1]


/* Payload for lcd_UpdateData(): OR together `LCD_MASK_*` bits. */
/* All values are passed in `lcd_update_t.u`; use `d_bool`, `d_uint32`, or `d_string` as appropriate for the bits set in @p mask. */
/* - **Board** (bits 0–1): `LCD_MASK_BOARD_MAC` → `u.d_uint8[0..5]`; `LCD_MASK_BOARD_UID` → `u.d_string[0]`. */
/* - **Link flags** (bits 4–7): `LCD_MASK_ETH_CONNECTED`, `LCD_MASK_WIFI_CONNECTED`, `LCD_MASK_MQTT_CONNECTED`, `LCD_MASK_BT_CONNECTED` → each uses `u.d_bool[0]` for that update. */
/* - **Ethernet IPv4** (bits 8–10): `LCD_MASK_ETH_IP` / `ETH_NETMASK` / `ETH_GW` → `u.d_uint32[0]` / `[1]` / `[2]` (network byte order). */
/* - **Ethernet MAC** (bit 11): `LCD_MASK_ETH_MAC` → `u.d_uint8[0..5]`. */
/* - **Wi-Fi IPv4** (bits 12–14): `LCD_MASK_WIFI_IP` / `WIFI_NETMASK` / `WIFI_GW` → `u.d_uint32[0]` / `[1]` / `[2]`. */
/* - **Wi-Fi MAC** (bit 15): `LCD_MASK_WIFI_MAC` → `u.d_uint8[0..5]`. */
/* - **Ambient** (bits 20–21): `LCD_MASK_AMBIENT_LUX` → `u.d_uint32[0]`; `LCD_MASK_AMBIENT_THRESHOLD` → `u.d_uint32[1]`. */
typedef struct {
  union {
    bool      d_bool[32];
    uint8_t   d_uint8[32];
    char      d_string[32];
    uint16_t  d_uint16[16];
    uint32_t  d_uint32[8];
    uint64_t  d_uint64[4];
  } u;
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

/**
 * @brief Thread-safe read of Ethernet link-up flag shown on the LCD status bar / dialogs.
 *
 * @return true if the last merged state is connected, false otherwise (or if mutex unavailable).
 */
bool lcd_GetEthLinkConnected(void);

/**
 * @brief Thread-safe read of Wi-Fi STA connected flag shown on the LCD status bar / dialogs.
 *
 * @return true if the last merged state is connected, false otherwise (or if mutex unavailable).
 */
bool lcd_GetWifiLinkConnected(void);

#endif /* __LCD_HELPER_H__ */
