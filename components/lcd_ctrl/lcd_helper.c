/**
 * @file lcd_helper.c
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Helper
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

#include "lcd_helper.h"

#include "ili9341v.h"

static const char* TAG = "ESP::LCD::HELPER";


esp_err_t lcd_InitHelper(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_HELPER_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  result = lcd_InitDisplay();

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
