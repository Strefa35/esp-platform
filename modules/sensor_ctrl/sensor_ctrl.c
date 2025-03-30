/**
 * @file sensor_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Sensor Controller
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
#include "sensor_ctrl.h"
#include "sensor_data.h"
#include "sensor_list.h"
#include "sensor_lut.h"

#include "err.h"
#include "lut.h"


#define SENSOR_TASK_NAME              "sensor-ctrl-task"
#define SENSOR_TASK_STACK_SIZE        4096
#define SENSOR_TASK_PRIORITY          12

#define SENSOR_MSG_MAX                10


static const char* TAG = "ESP::SENSOR";


static QueueHandle_t      sensor_msg_queue = NULL;
static TaskHandle_t       sensor_task_id = NULL;
static SemaphoreHandle_t  sensor_sem_id = NULL;


static esp_err_t parseData(const sensor_data_t* data) {
  esp_err_t result = ESP_OK;
  
  ESP_LOGI(TAG, "++%s(data: %p)", __func__, data);
  ESP_LOGD(TAG, "[%s] DATA: type: %d [%s], dtype: %d [%s]", __func__, 
        data->type, GET_SENSOR_TYPE_NAME(data->type), 
        data->dtype, GET_SENSOR_DATA_NAME(data->dtype));

  switch (data->type) {
    case SENSOR_TYPE_TSL2561: {
      if (data->dtype == SENSOR_DATA_LUX) {
        ESP_LOGD(TAG, "[%s] LUX: %ldlx", __func__, data->u.int32[0]);
      }
      break;
    }
    default: {
      ESP_LOGW(TAG, "[%s] Unknown sensor: type: %d, dtype: %d", __func__, data->type, data->dtype);
      result = ESP_FAIL;
      break;
    }
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


static esp_err_t sensorCb(const sensor_data_t* data) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(data: %p)", __func__, data);
  if (data) {
    result = parseData(data);
  } else {
    ESP_LOGE(TAG, "[%s] data = NULL", __func__);
    result = ESP_ERR_INVALID_ARG;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}


static esp_err_t initSensors(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  for (int idx = 0; idx < SENSOR_LIST_CNT; ++idx) {
    if (sensor_list[idx].init) {
      result = sensor_list[idx].init(sensorCb);
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] tsl2561_InitSensor() failed.", __func__);
      }
    }
  }
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
    if(xQueueReceive(sensor_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
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
  if (sensor_sem_id) {
    xSemaphoreGive(sensor_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(sensor_msg_queue, msg, (TickType_t) 0) != pdPASS) {
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
  sensor_msg_queue = xQueueCreate(SENSOR_MSG_MAX, sizeof(msg_t));
  if (sensor_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  sensor_sem_id = xSemaphoreCreateCounting(1, 0);
  if (sensor_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(taskFn, SENSOR_TASK_NAME, SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, &sensor_task_id);
  if (sensor_task_id == NULL)
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
  if (sensor_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_SENSOR_CTRL,
      .to = REG_SENSOR_CTRL,
    };
    result = send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(sensor_sem_id, portMAX_DELAY);

    vSemaphoreDelete(sensor_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (sensor_msg_queue) {
    vQueueDelete(sensor_msg_queue);
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
esp_err_t SensorCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_SENSOR_CTRL_LOG_LEVEL);

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
esp_err_t SensorCtrl_Done(void) {
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
esp_err_t SensorCtrl_Run(void) {
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
esp_err_t SensorCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
