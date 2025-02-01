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
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "manager.h"
#include "mgr_reg.h"

#include "mgr_reg_list.h"


#define MGR_MSG_MAX           20

#define MGR_TASK_NAME         "mgr-task"
#define MGR_TASK_STACK_SIZE   4096
#define MGR_TASK_PRIORITY     10


static const char* TAG = "EWHC::MANAGER";


static int mgr_modules_cnt = MGR_REG_LIST_CNT;

static QueueHandle_t      mgr_msg_queue = NULL;
static TaskHandle_t       mgr_task_id = NULL;
static SemaphoreHandle_t  mgr_sem_id = NULL;


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

static mgr_reg_t* mgr_FindTopic(const char* topic) {
  mgr_reg_t* slot_ptr = NULL;

  ESP_LOGI(TAG, "++%s(topic: %s)", __func__, topic);
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    if (strncmp(topic, mgr_reg_list[idx].topic, strlen(mgr_reg_list[idx].topic)) == 0) {
      ESP_LOGD(TAG, "[%s] Topic: '%s' found.", __func__, topic);
      slot_ptr = &mgr_reg_list[idx];
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - slot_ptr: %p", __func__, slot_ptr);
  return slot_ptr;
}

static esp_err_t mgr_MsgParse(const msg_t* msg) {
  mgr_reg_t* slot_ptr = NULL;
  esp_err_t result = ESP_ERR_NOT_FOUND;

  ESP_LOGI(TAG, "++%s(type: %d, topic: '%s')", __func__, msg->type, msg->topic);
  slot_ptr = mgr_FindTopic(msg->topic);
  if (slot_ptr && slot_ptr->send_fn) {
    result = slot_ptr->send_fn(msg);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Manager task's function
 * 
 * @param param 
 */
static void mgr_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(mgr_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d, topic: '%s'", __func__, msg.type, msg.topic);
      result = mgr_MsgParse(&msg);
      if (result != ESP_OK) {
        // TODO - Send Error to the Broker
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  if (mgr_sem_id) {
    xSemaphoreGive(mgr_sem_id);
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
  if (mgr_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  mgr_sem_id = xSemaphoreCreateCounting(1, 0);
  if (mgr_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }
 
  /* Initialization manager thread */
  xTaskCreate(mgr_TaskFn, MGR_TASK_NAME, MGR_TASK_STACK_SIZE, NULL, MGR_TASK_PRIORITY, &mgr_task_id);
  if (mgr_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGD(TAG, "Modules to register: %d", mgr_modules_cnt);
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
  if (mgr_sem_id) {
    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake...", __func__);
    xSemaphoreTake(mgr_sem_id, portMAX_DELAY);
    ESP_LOGD(TAG, "[%s] xSemaphoreTake was released.", __func__);
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
  if (mgr_task_id) {
    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (mgr_sem_id) {
    vSemaphoreDelete(mgr_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);
  }
  if (mgr_msg_queue) {
    vQueueDelete(mgr_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t MGR_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(mgr_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, topic: '%s'", __func__, msg->type, msg->topic);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}