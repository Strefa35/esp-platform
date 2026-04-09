/**
 * @file main.c
 * @author A.Czerwinski@pistacje.net
 * @brief Application entry point
 * @version 0.1
 * @date 2026-03-29
 *
 * @copyright Copyright (c) 2025 4Embedded.Systems
 *
 */
#include <stdio.h>
#include <stdbool.h>

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_ctrl.h"
#include "mgr_ctrl.h"
#include "mem.h"
#include "tools.h"


static const char* TAG = "ESP::MAIN";

/**
 * @brief Main function
 * 
 */
void app_main(void) {
  esp_err_t result;

  esp_log_level_set(TAG, CONFIG_MAIN_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);

  MEM_CHECK({
    result = mem_Init(CONFIG_MAIN_MEMORY_PERIODIC_MONITOR_ENABLE);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s]() - mem_Init() failed", __func__);
    }
  });

  MEM_CHECK(mem_LogSnapshot(__func__, "app_main_begin"));

  result = tools_Init();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - tools_Init() failed", __func__);
  }
  MEM_CHECK(mem_LogSnapshot(__func__, "after_tools_init"));

  result = NVS_Init();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - NVS_Init() failed", __func__);
  }
  MEM_CHECK(mem_LogSnapshot(__func__, "after_nvs_init"));

  result = MGR_Init();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - MGR_Init() failed", __func__);
  }
  MEM_CHECK(mem_LogSnapshot(__func__, "after_mgr_init"));

  result = MGR_Run();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - MGR_Run() failed", __func__);
  }
  MEM_CHECK(mem_LogSnapshot(__func__, "after_mgr_run"));

  result = MGR_Done();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - MGR_Done() failed", __func__);
  }

  result = NVS_Done();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - NVS_Done() failed", __func__);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
}
