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
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_err.h"

#include "lcd_hw.h"

#include "ili9341v.h"
#include "ns2009.h"


#define LCD_HW_TASK_NAME              "lcd-hw-task"
#define LCD_HW_TASK_STACK_SIZE        2048
#define LCD_HW_TASK_PRIORITY          10


static const char* TAG = "ESP::LCD::HW";


static TaskHandle_t       lcd_hw_task_id = NULL;

static const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

static void lcdhw_TaskFn(void* param) {
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  while (loop) {
    ns2009_touch_t touch = {0, 0, 0};

    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    result = ns2009_GetTouch(&touch);
    if (result == ESP_OK) {
      ESP_LOGD(TAG, "[%s] Touch", __func__);
    } else {
      ESP_LOGD(TAG, "[%s] ns2009_GetTouch() - result: %d", __func__, result);
    }
    vTaskDelay( xDelay );
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

esp_err_t lcd_InitHw(lcd_t* lcd_ptr) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_HW_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  result = lcd_InitDisplayHw(lcd_ptr);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_InitDisplayHw() - result: %d", __func__, result);
    return result;
  }

  ns2009_res_t res = {320, 240};

  result = ns2009_Init(&res);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] ns2009_Init() - result: %d", __func__, result);
    return result;
  }

  /* Initialization thread */
  xTaskCreate(lcdhw_TaskFn, LCD_HW_TASK_NAME, LCD_HW_TASK_STACK_SIZE, NULL, LCD_HW_TASK_PRIORITY, &lcd_hw_task_id);
  if (lcd_hw_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }


  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
