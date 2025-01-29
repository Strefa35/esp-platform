/**
 * Copyright (C) 2025 4Embeddes.Systems
 * All rights reserved. 
 * 
 * Find details copyright statement at "LICENSE" file.
 */
#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"

#include "manager.h"


static const char* TAG = "EWHC::MAIN";

/**
 * @brief Main function
 * 
 */
void app_main(void) {
  bool result;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = MGR_Init();
  if (result) {
    result = MGR_Run();
    MGR_Done();
  } else {
    ESP_LOGI(TAG, "[%s]() - failed", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
}
