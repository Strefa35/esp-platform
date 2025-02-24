/**
 * @file ns2009.c
 * @author A.Czerwinski@pistacje.net
 * @brief TFT LCD Single Chip Driver
 * @version 0.1
 * @date 2025-02-22
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <stdio.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"

#include "ns2009.h"


static const char* TAG = "ESP::LCD::NS2009";

static esp_err_t lcd_SetupTouchHw(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t lcd_InitTouchHw(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_NS2009_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcd_SetupTouchHw(lcd_ptr);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
