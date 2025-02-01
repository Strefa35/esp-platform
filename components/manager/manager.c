/**
 * @file manager.c
 * @author A.Czerwinski@pistacje.net
 * @brief Manager module
 * @version 0.1
 * @date 2025-01-28
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "msg.h"
#include "manager.h"
#include "mgr_reg.h"

#include "mgr_reg_list.h"


#define MGR_MSG_MAX           20

#define MGR_TASK_NAME         "mgr-task"
#define MGR_TASK_STACK_SIZE   4096
#define MGR_TASK_PRIORITY     10


static const char* TAG = "EWHC::MANAGER";


static int mgr_modules_cnt = MGR_REG_LIST_CNT;

static QueueHandle_t mgr_msg_queue = NULL;
static TaskHandle_t mgr_TaskId = NULL;


static esp_err_t mgr_Init(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, topic: %s)", __func__, id, mgr_reg_list[id].topic);
  if (mgr_reg_list[id].init_fn) {
    result = mgr_reg_list[id].init_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Run(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, topic: %s)", __func__, id, mgr_reg_list[id].topic);
  if (mgr_reg_list[id].run_fn) {
    result = mgr_reg_list[id].run_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Done(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, topic: %s)", __func__, id, mgr_reg_list[id].topic);
  if (mgr_reg_list[id].done_fn) {
    result = mgr_reg_list[id].done_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static void mgr_TaskFn(void* param) {
  bool loop = true;

  ESP_LOGI(TAG, "++%s()", __func__);
  while (loop) {
    ESP_LOGI(TAG, "[%s] Loop...", __func__);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Init manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  mgr_msg_queue = xQueueCreate(MGR_MSG_MAX, sizeof(msg_t));
 
  /* Initialization manager thread */
  xTaskCreate(mgr_TaskFn, MGR_TASK_NAME, MGR_TASK_STACK_SIZE, NULL, MGR_TASK_PRIORITY, &mgr_TaskId);

  ESP_LOGI(TAG, "Modules to register: %d", mgr_modules_cnt);
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    result = mgr_Init(idx);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    result = mgr_Run(idx);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  for (int idx = mgr_modules_cnt - 1; idx >= 0; --idx) {
    result = mgr_Done(idx);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
