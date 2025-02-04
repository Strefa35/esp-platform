/**
 * Copyright (C) 2025 4Embeddes.Systems
 * All rights reserved. 
 * 
 * Find details copyright statement at "LICENSE" file.
 */
#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"

#include "tags.h"

#include "mgr_ctrl.h"


static const char* TAG = MAIN_TAG;

/**
 * @brief Main function
 * 
 */
void app_main(void) {
  esp_err_t result;

  esp_log_level_set(MAIN_TAG, CONFIG_MAIN_LOG_LEVEL);
  esp_log_level_set(MGR_CTRL_TAG, CONFIG_MGR_CTRL_LOG_LEVEL);

#ifdef CONFIG_ETH_CTRL_ENABLE
  esp_log_level_set(ETH_CTRL_TAG, CONFIG_ETH_CTRL_LOG_LEVEL);
#endif

#ifdef CONFIG_CLI_CTRL_ENABLE
  esp_log_level_set(CLI_CTRL_TAG, CONFIG_CLI_CTRL_LOG_LEVEL);
#endif

#ifdef CONFIG_GPIO_CTRL_ENABLE
  esp_log_level_set(GPIO_CTRL_TAG, CONFIG_GPIO_CTRL_LOG_LEVEL);
#endif

#ifdef CONFIG_POWER_CTRL_ENABLE
  esp_log_level_set(POWER_CTRL_TAG, CONFIG_POWER_CTRL_LOG_LEVEL);
#endif

#ifdef CONFIG_MQTT_CTRL_ENABLE
  esp_log_level_set(MQTT_CTRL_TAG, CONFIG_MQTT_CTRL_LOG_LEVEL);
#endif

  ESP_LOGI(TAG, "++%s()", __func__);
  result = MGR_Init();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - MGR_Init() failed", __func__);
  }

  result = MGR_Run();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - MGR_Run() failed", __func__);
  }

  result = MGR_Done();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s]() - MGR_Done() failed", __func__);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
}
