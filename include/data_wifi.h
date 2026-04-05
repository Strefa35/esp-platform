/**
 * @file data_wifi.h
 * @brief Neutral Wi-Fi data layouts for `data_type_e` blobs (no ESP-IDF Wi-Fi headers).
 *
 * Lives under the project `include/` tree so any component with `../../include` (or
 * equivalent) can use these types without adding a CMake dependency on `wifi_ctrl`.
 * Semantics are owned by `wifi_ctrl` (producer); this file is only the shared contract.
 *
 * Used with `DATA_TYPE_WIFI_SCAN_LIST` and related kinds in `data.h`. Prefer
 * `#include "data_types.h"` to pull `data.h` and all neutral DTO headers together.
 */

#ifndef __DATA_WIFI_H__
#define __DATA_WIFI_H__

#include <stdint.h>

typedef enum {
  WIFI_UI_SEC_OPEN = 0,
  WIFI_UI_SEC_WEP,
  WIFI_UI_SEC_WPA_PSK,
  WIFI_UI_SEC_WPA2_PSK,
  WIFI_UI_SEC_WPA3_PSK,
  WIFI_UI_SEC_OTHER,
} wifi_ui_security_e;

#define WIFI_UI_SSID_MAX 33u

typedef struct {
  char ssid[WIFI_UI_SSID_MAX];
  int8_t rssi;
  uint8_t channel;
  wifi_ui_security_e security;
} wifi_ui_ap_row_t;

#endif /* __DATA_WIFI_H__ */
