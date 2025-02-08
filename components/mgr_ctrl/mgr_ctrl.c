/**
 * @file mgr_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Manager Controller
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

#include "sdkconfig.h"

#include "tags.h"

#include "mgr_ctrl.h"
#include "mgr_reg.h"

#include "mgr_reg_list.h"

#include "lut.h"


#define MGR_TASK_NAME         "mgr-task"
#define MGR_TASK_STACK_SIZE   4096
#define MGR_TASK_PRIORITY     10

#define MGR_MSG_MAX           40


static const char* TAG = MGR_CTRL_TAG;


static int mgr_modules_cnt = MGR_REG_LIST_CNT;

static QueueHandle_t      mgr_msg_queue = NULL;
static TaskHandle_t       mgr_task_id = NULL;
static SemaphoreHandle_t  mgr_sem_id = NULL;


static esp_err_t mgr_Init(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].init_fn) {
    result = mgr_reg_list[id].init_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Done(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].done_fn) {
    result = mgr_reg_list[id].done_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Run(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].run_fn) {
    result = mgr_reg_list[id].run_fn();
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(mgr_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
        msg->type, GET_MSG_TYPE_NAME(msg->type),
        msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_ParseEthPayload(const msg_type_e type, const payload_eth_t* eth) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s])", __func__, type, GET_MSG_TYPE_NAME(type));
  switch (type) {
    case MSG_TYPE_ETH_EVENT: {
      ESP_LOGD(TAG, "[%s] Event: %d [%s]", __func__, eth->u.event.id, GET_ETH_EVENT_NAME(eth->u.event.id));
      break;
    }

    case MSG_TYPE_ETH_IP: {
      uint8_t* addr = NULL;

      addr = (uint8_t*) &(eth->u.info.ip);
      ESP_LOGD(TAG, "[%s]   IP: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      addr = (uint8_t*) &(eth->u.info.mask);
      ESP_LOGD(TAG, "[%s] MASK: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      addr = (uint8_t*) &(eth->u.info.gw);
      ESP_LOGD(TAG, "[%s]   GW: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );
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

static esp_err_t mgr_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type, GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);
  switch (msg->from) {
    case REG_ETH_CTRL: {
      result = mgr_ParseEthPayload(msg->type, &(msg->payload.eth));
      break;
    }
    case REG_CLI_CTRL: {
      break;
    }
    case REG_GPIO_CTRL: {
      break;
    }
    case REG_POWER_CTRL: {
      break;
    }
    case REG_MQTT_CTRL: {
      break;
    }
    default: {
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_NotifyCtrl(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type,  GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    if (msg->to & mgr_reg_list[idx].type) {
      if (mgr_reg_list[idx].send_fn) {
        result = mgr_reg_list[idx].send_fn(msg);
      }
    }
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
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);

      /* First, parse message in manager */
      if (msg.to & REG_MGR_CTRL) {
        mgr_ParseMsg(&msg);
      }

      /* Now, notify specific (or all) registered controller */
      result = mgr_NotifyCtrl(&msg);

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

/**
 * @brief Run manager
 * 
 * \return esp_err_t 
 */
esp_err_t MGR_Run(void) {
  msg_t msg = { 
    .type = MSG_TYPE_MGR_LIST, 
    .from = REG_MGR_CTRL, 
    .to = REG_MQTT_CTRL,
  };
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  sprintf(msg.payload.mgr.msg, "[");
  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    strcat(msg.payload.mgr.msg, mgr_reg_list[idx].name);
    if (idx < mgr_modules_cnt - 1) {
      strcat(msg.payload.mgr.msg, ",");
    }
    result = mgr_Run(idx);
  }
  strcat(msg.payload.mgr.msg, "]");
  ESP_LOGD(TAG, "[%s] List of modules: %s", __func__, msg.payload.mgr.msg);
  if (mgr_sem_id) {

    mgr_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake...", __func__);
    xSemaphoreTake(mgr_sem_id, portMAX_DELAY);
    ESP_LOGD(TAG, "[%s] xSemaphoreTake was released.", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t MGR_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mgr_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
