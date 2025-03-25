/**
 * @file sensors_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Sensors Controller
 * @version 0.1
 * @date 2025-03-24
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "err.h"
#include "msg.h"
#include "sensors_ctrl.h"

#ifdef CONFIG_SENSOR_TSL2561_ENABLE
  #include "sensor_tsl2561.h"
#endif

#include "err.h"
#include "lut.h"


#define SENSORS_TASK_NAME             "sensors-task"
#define SENSORS_TASK_STACK_SIZE       4096
#define SENSORS_TASK_PRIORITY         12

#define SENSORS_MSG_MAX               10


static const char* TAG = "ESP::SENSORS";


static QueueHandle_t      sensors_msg_queue = NULL;
static TaskHandle_t       sensors_task_id = NULL;
static SemaphoreHandle_t  sensors_sem_id = NULL;

static esp_err_t initSensors(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

#ifdef CONFIG_SENSOR_TSL2561_ENABLE
  result = tsl2561_InitSensor();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] tsl2561_InitSensor() failed.", __func__);
  }
#endif

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t parseMsg(const msg_t* msg) {
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
 * @brief Sensors task's function
 * 
 * @param param 
 */
static void taskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  initSensors();
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(sensors_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
      
      result = parseMsg(&msg);
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
  if (sensors_sem_id) {
    xSemaphoreGive(sensors_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(sensors_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  sensors_msg_queue = xQueueCreate(SENSORS_MSG_MAX, sizeof(msg_t));
  if (sensors_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  sensors_sem_id = xSemaphoreCreateCounting(1, 0);
  if (sensors_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(taskFn, SENSORS_TASK_NAME, SENSORS_TASK_STACK_SIZE, NULL, SENSORS_TASK_PRIORITY, &sensors_task_id);
  if (sensors_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (sensors_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_SENSORS_CTRL,
      .to = REG_SENSORS_CTRL,
    };
    result = send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(sensors_sem_id, portMAX_DELAY);

    vSemaphoreDelete(sensors_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (sensors_msg_queue) {
    vQueueDelete(sensors_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init Sensors controller
 * 
 * \return esp_err_t 
 */
esp_err_t SensorsCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SENSORS_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done Sensors controller
 * 
 * \return esp_err_t 
 */
esp_err_t SensorsCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run Sensors controller
 * 
 * \return esp_err_t 
 */
esp_err_t SensorsCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the Sensors controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t SensorsCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
