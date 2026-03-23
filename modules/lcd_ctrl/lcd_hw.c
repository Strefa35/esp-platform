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
#include <stdbool.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "ili9341v.h"
#include "lcd_hw.h"
#include "ns2009.h"


static const char* TAG = "ESP::LCD::HW";


/**
 * @brief Initializes LCD display and touch hardware layers.
 *
 * @param lcd_ptr LCD context structure to initialize.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_InitHw(lcd_t* lcd_ptr) {
  esp_log_level_set(TAG, CONFIG_LCD_HW_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_RETURN_ON_ERROR(lcd_InitDisplayHw(lcd_ptr), TAG, "lcd_InitDisplayHw failed");

  ns2009_res_t res = {
    .h = (uint32_t) lcd_ptr->h_res,
    .v = (uint32_t) lcd_ptr->v_res,
  };
  ESP_RETURN_ON_ERROR(ns2009_Init(&res), TAG, "ns2009_Init failed");

  ESP_LOGI(TAG, "--%s()", __func__);
  return ESP_OK;
}

/**
 * @brief Deinitializes LCD touch and display hardware layers.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t lcd_DoneHw(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  esp_err_t result_touch = ns2009_Done();
  esp_err_t result_lcd = lcd_DoneDisplayHw();

  esp_err_t result = (result_touch != ESP_OK) ? result_touch : result_lcd;
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
