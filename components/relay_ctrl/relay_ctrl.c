/**
 * @file relay_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Relay Controller
 * @version 0.1
 * @date 2025-02-12
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "tags.h"

#include "err.h"
#include "msg.h"
#include "relay_ctrl.h"

#include "err.h"
#include "lut.h"


#define RELAY_TASK_NAME          "cfg-task"
#define RELAY_TASK_STACK_SIZE    4096
#define RELAY_TASK_PRIORITY      10

#define RELAY_MSG_MAX            40


static const char* TAG = RELAY_CTRL_TAG;


static QueueHandle_t      relay_msg_queue = NULL;
static TaskHandle_t       relay_task_id = NULL;
static SemaphoreHandle_t  relay_sem_id = NULL;


static esp_err_t relayctrl_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type, GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);

  switch (msg->type) {
    case MSG_TYPE_INIT: {
      result = ESP_TASK_INIT;
      break;
    }
    case MSG_TYPE_DONE: {
      result = ESP_TASK_DONE;
      break;
    }
    case MSG_TYPE_RUN: {
      result = ESP_TASK_RUN;
      break;
    }
    default: {
      result = ESP_FAIL;
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Relay task's function
 * 
 * @param param 
 */
static void relayctrl_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(relay_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
      
      result = relayctrl_ParseMsg(&msg);
      if (result == ESP_TASK_DONE) {
        loop = false;
        result = ESP_OK;
      }

      if (result != ESP_OK) {
        // TODO - Send Error to the Broker
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  if (relay_sem_id) {
    xSemaphoreGive(relay_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t relayctrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(relay_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t relayctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  relay_msg_queue = xQueueCreate(RELAY_MSG_MAX, sizeof(msg_t));
  if (relay_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  relay_sem_id = xSemaphoreCreateCounting(1, 0);
  if (relay_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(relayctrl_TaskFn, RELAY_TASK_NAME, RELAY_TASK_STACK_SIZE, NULL, RELAY_TASK_PRIORITY, &relay_task_id);
  if (relay_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t relayctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (relay_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_RELAY_CTRL,
      .to = REG_RELAY_CTRL,
    };
    result = relayctrl_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(relay_sem_id, portMAX_DELAY);

    vSemaphoreDelete(relay_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (relay_msg_queue) {
    vQueueDelete(relay_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t relayctrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init Relay controller
 * 
 * \return esp_err_t 
 */
esp_err_t RelayCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = relayctrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done Relay controller
 * 
 * \return esp_err_t 
 */
esp_err_t RelayCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = relayctrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run Relay controller
 * 
 * \return esp_err_t 
 */
esp_err_t RelayCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = relayctrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the Relay controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t RelayCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = relayctrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
