/**
 * @file template_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Template Controller
 * @version 0.1
 * @date 2025-01-31
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "sdkconfig.h"

#include "err.h"
#include "msg.h"
#include "template_ctrl.h"

#include "err.h"
#include "lut.h"


#define TEMPLATE_TASK_NAME          "template-task"
#define TEMPLATE_TASK_STACK_SIZE    4096
#define TEMPLATE_TASK_PRIORITY      12

#define TEMPLATE_MSG_MAX            10


static const char* TAG = "ESP::TEMPLATE";


static QueueHandle_t      template_msg_queue = NULL;
static TaskHandle_t       template_task_id = NULL;
static SemaphoreHandle_t  template_sem_id = NULL;

static data_uid_t         esp_uid = {0};


/**
 * @brief Parse json format payload
 *
 * @param json_str - json message
 *
 * {
 *    "operation": "set",
 *
 *     ...
 *
 * }
 *
 * {
 *   "operation": "get",
 *
 *    ...
 *
 * }
 *
 * @return esp_err_t
 */
static esp_err_t parseMqttData(const char* json_str) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(json_str: '%s')", __func__, json_str);
  cJSON* root = cJSON_Parse(json_str);
  if (root) {
    const cJSON* operation = cJSON_GetObjectItem(root, "operation");
    if (operation) {
      const char* o_str = cJSON_GetStringValue(operation);
      ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);

      if (strcmp(o_str, "set") == 0) {

      } else if (strcmp(o_str, "get") == 0) {

      } else {
        ESP_LOGW(TAG, "[%s] Unknown operation: '%s'", __func__, o_str);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Bad data format. Missing operation field.", __func__);
      ESP_LOGE(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(root));
    }
    cJSON_Delete(root);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t templatectrl_ParseMsg(const msg_t* msg) {
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

    case MSG_TYPE_MGR_UID: {
      memcpy(esp_uid, msg->payload.mgr.uid, strlen(msg->payload.mgr.uid) + 1);
      ESP_LOGD(TAG, "[%s] UID: '%s'", __func__, esp_uid);
      break;
    }

    case MSG_TYPE_MQTT_EVENT: {
      data_mqtt_event_e event_id = msg->payload.mqtt.u.event_id;
      ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
      break;
    }

    case MSG_TYPE_MQTT_DATA: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      ESP_LOGD(TAG, "[%s] topic: '%s'", __func__, data_ptr->topic);
      ESP_LOGD(TAG, "[%s]   msg: '%s'", __func__, data_ptr->msg);
      result = parseMqttData(data_ptr->msg);
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
 * @brief Template task's function
 * 
 * @param param 
 */
static void templatectrl_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(template_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
      
      result = templatectrl_ParseMsg(&msg);
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
  if (template_sem_id) {
    xSemaphoreGive(template_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t templatectrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(template_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t templatectrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  template_msg_queue = xQueueCreate(TEMPLATE_MSG_MAX, sizeof(msg_t));
  if (template_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  template_sem_id = xSemaphoreCreateCounting(1, 0);
  if (template_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(templatectrl_TaskFn, TEMPLATE_TASK_NAME, TEMPLATE_TASK_STACK_SIZE, NULL, TEMPLATE_TASK_PRIORITY, &template_task_id);
  if (template_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t templatectrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (template_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_XXX_CTRL,
      .to = REG_XXX_CTRL,
    };
    result = templatectrl_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(template_sem_id, portMAX_DELAY);

    vSemaphoreDelete(template_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (template_msg_queue) {
    vQueueDelete(template_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t templatectrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init Template controller
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_TEMPLATE_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = templatectrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done Template controller
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = templatectrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run Template controller
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = templatectrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the Template controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t TemplateCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = templatectrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
