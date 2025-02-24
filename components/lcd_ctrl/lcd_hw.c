/**
 * @file lcd_hw.c
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Hardware
 * @version 0.1
 * @date 2025-02-23
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <stdio.h>
#include <stdbool.h>
 
#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_err.h"

#include "lcd_hw.h"

#include "ili9341v.h"
#include "ns2009.h"

static const char* TAG = "ESP::LCD::HW";


esp_err_t lcd_InitHw(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_HW_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  result = lcd_InitDisplayHw(lcd_ptr);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_InitDisplayHw() - result: %d", __func__, result);
    return result;
  }

  result = lcd_InitTouchHw(lcd_ptr);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_InitTouchHw() - result: %d", __func__, result);
    return result;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
