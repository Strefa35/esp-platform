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

#include "manager.h"


static const char* TAG = "EWHC::MAIN";

/**
 * @brief Main function
 * 
 */
void app_main(void) {
  esp_err_t result;

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
