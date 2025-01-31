/**
 * @file manager.c
 * @author A.Czerwinski@pistacje.net
 * @brief 
 * @version 0.1
 * @date 2025-01-28
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "manager.h"
#include "mgr_reg.h"
#include "mgr_reg_list.h"


static const char* TAG = "EWHC::MANAGER";


/**
 * @brief Init manager
 * 
 * @return true 
 * @return false 
 */
bool MGR_Init(void) {
  bool result = true;

  ESP_LOGI(TAG, "++%s()", __func__);
  // for (int idx = 0; idx < MGR_REG_LIST_CNT; ++idx) {
  //   if (mgr_reg_list[idx].init_fn) {
  //     mgr_reg_list[idx].init_fn();
  //   }
  // }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run manager
 * 
 * @return true 
 * @return false 
 */
bool MGR_Run(void) {
  bool loop = true;
  bool result = false;

  ESP_LOGI(TAG, "++%s()", __func__);
  while (loop) {
    vTaskDelay(-1);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done manager
 * 
 * @return true 
 * @return false 
 */
bool MGR_Done(void) {
  bool result = true;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
