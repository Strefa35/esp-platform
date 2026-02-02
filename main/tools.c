/**
 * @file tools.c
 * @author A.Czerwinski@pistacje.net
 * @brief Tools
 * @version 0.1
 * @date 2026-02-01
 * 
 * @copyright Copyright (c) 2026 4Embedded.Systems
 * 
 */
#include <string.h>

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_mac.h"

#include "tools.h"

#include "err.h"
#include "lut.h"
#include "sys_lut.h"


static const char* TAG = "ESP::TOOLS";


/**
 * @brief Get MAC address
 * 
 * @param mac_ptr Pointer to a buffer where the MAC address will be stored
 * @param type The type of MAC address to retrieve
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t tools_GetMacAddress(uint8_t *mac_ptr, esp_mac_type_t type) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s])", __func__, type, GET_ESP_MAC_TYPE_NAME(type));
  if (mac_ptr == NULL) {
    ESP_LOGE(TAG, "MAC address buffer is NULL");
    result = ESP_ERR_INVALID_ARG;
  } else {
    result = esp_read_mac(mac_ptr, type);
    if (result == ESP_OK) {
      ESP_LOGD(TAG, "MAC Address [%s]: %02X:%02X:%02X:%02X:%02X:%02X",
               GET_ESP_MAC_TYPE_NAME(type),
               mac_ptr[0], mac_ptr[1], mac_ptr[2], mac_ptr[3], mac_ptr[4], mac_ptr[5]);
    } else {
      ESP_LOGE(TAG, "Failed to read MAC address, error: %d", result);
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initialize tools module
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure
 */
esp_err_t tools_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_TOOLS_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
