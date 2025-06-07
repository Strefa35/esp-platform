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

#include "cJSON.h"

#include "sdkconfig.h"

#include "err.h"
#include "msg.h"
#include "types.h"
#include "mgr_ctrl.h"
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

static data_uid_t         esp_uid = {0};


static esp_err_t sensorCb(cJSON* data, void* param) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(data: %p, param: %p)", __func__, data, param);
  if (data) {
    msg_t msg = {
      .type = MSG_TYPE_MQTT_PUBLISH,
      .from = REG_SENSOR_CTRL,
      .to = REG_MQTT_CTRL,
    };
    uint32_t idx = (uint32_t) param;

    if (idx >= SENSOR_LIST_CNT) {
      ESP_LOGW(TAG, "[%s] Passed param: %ld is wrong", __func__, idx);
      return result;
    }

    /* create root of json for event */
    cJSON* event = cJSON_CreateObject();
    if (event == NULL)
    {
      ESP_LOGW(TAG, "[%s] cJSON_CreateObject() failed", __func__);
      return ESP_FAIL;
    }

    cJSON_AddStringToObject(event, "operation", "event");
    cJSON_AddStringToObject(event, "sensor", sensor_list[idx].name);
    cJSON_AddItemToObject(event, "data", data);

    int ret = -1;
    if ((ret = cJSON_PrintPreallocated(event, msg.payload.mqtt.u.data.msg, DATA_MSG_SIZE, 0)) == 1) {
      /* add topic -> ESP/12AB34/event/sensor */
      snprintf(msg.payload.mqtt.u.data.topic, DATA_MSG_SIZE, "%s/event/sensor", esp_uid);

      result = MGR_Send(&msg);
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] MGR_Send() - Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] cJSON_PrintPreallocated() - Error: %d", __func__, ret);
    }
    cJSON_Delete(event);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t initSensors(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  for (uint32_t idx = 0; idx < SENSOR_LIST_CNT; ++idx) {
    if (sensor_list[idx].init) {
      result = sensor_list[idx].init(sensorCb, (void*) idx);
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] tsl2561_InitSensor() failed.", __func__);
      }
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static sensor_reg_t* findSensor(const char* name) {
  sensor_reg_t* sensor_slot_ptr = NULL;

  ESP_LOGI(TAG, "++%s(name: '%s')", __func__, name);
  for (int idx = 0; idx < SENSOR_LIST_CNT; ++idx) {
    if (strcmp(sensor_list[idx].name, name) == 0) {
      ESP_LOGD(TAG, "[%s] Sensor '%s' has been found.", __func__, name);
      sensor_slot_ptr = &sensor_list[idx];
      break;
    }
  }
  ESP_LOGI(TAG, "--%s() - sensor_slot_ptr: %p", __func__, sensor_slot_ptr);
  return sensor_slot_ptr;
}

static esp_err_t publishResponse(cJSON* response, msg_t* msg) {
  int ret = -1;
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(response: %p, msg: %p)", __func__, response, msg);
  if ((ret = cJSON_PrintPreallocated(response, msg->payload.mqtt.u.data.msg, DATA_MSG_SIZE, 0)) == 1) {
    /* add topic -> ESP/12AB34/res/sensor */
    snprintf(msg->payload.mqtt.u.data.topic, DATA_MSG_SIZE, "%s/res/sensor", esp_uid);

    result = MGR_Send(msg);
    if (result != ESP_OK) {
      ESP_LOGE(TAG, "[%s] MGR_Send() - Error: %d", __func__, result);
    }
  } else {
    ESP_LOGE(TAG, "[%s] cJSON_PrintPreallocated() - Error: %d", __func__, ret);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t publishError(const char* error_msg) {
  cJSON *response;
  msg_t msg = {
    .type = MSG_TYPE_MQTT_PUBLISH,
    .from = REG_SENSOR_CTRL,
    .to = REG_MQTT_CTRL,
  };
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(error_msg: '%s')", __func__, error_msg);

  /* create root of json for response */
  response = cJSON_CreateObject();
  if (response == NULL)
  {
    ESP_LOGW(TAG, "[%s] cJSON_CreateObject() failed", __func__);
    return ESP_FAIL;
  }

  cJSON_AddStringToObject(response, "operation", "response");
  cJSON_AddStringToObject(response, "status", "error");
  cJSON_AddStringToObject(response, "message", error_msg);

  /* Send json to the thread */
  result = publishResponse(response, &msg);

  cJSON_Delete(response);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t useSensor(const char* name, const char* op_str, const cJSON* data) {
  sensor_reg_t* sensor = findSensor(name);
  operation_type_e op = convertOperation(op_str);
  cJSON *response;
  msg_t msg = {
    .type = MSG_TYPE_MQTT_PUBLISH,
    .from = REG_SENSOR_CTRL,
    .to = REG_MQTT_CTRL,
  };
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(name: '%s', operation: '%s')", __func__, name, op_str);
  if (sensor == NULL) {
    char error_msg[30] = "";

    ESP_LOGW(TAG, "[%s] Unknown sensor: '%s'", __func__, name);
    snprintf(error_msg, 30, "Unknown sensor: '%s'", name);
    result = publishError(error_msg);
    return ESP_FAIL;
  }
  /* create root of json for response */
  response = cJSON_CreateObject();
  if (response == NULL)
  {
    ESP_LOGW(TAG, "[%s] cJSON_CreateObject() failed", __func__);
    return ESP_FAIL;
  }

  cJSON_AddStringToObject(response, "operation", "response");
  cJSON_AddStringToObject(response, "sensor", name);
  cJSON* status_obj = cJSON_AddStringToObject(response, "status", "ok");

  switch (op) {
    case OP_TYPE_SET: {
      if (sensor->set) {
        result = sensor->set(data, response);
        ESP_LOGD(TAG, "[%s] OP_TYPE_SET:", __func__);
        ESP_LOGD(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(data));
        ESP_LOGD(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(response));
      }
      break;
    }
    case OP_TYPE_GET: {
      if (sensor->get) {
        result = sensor->get(data, response);
        ESP_LOGD(TAG, "[%s] OP_TYPE_GET:", __func__);
        ESP_LOGD(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(data));
        ESP_LOGD(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(response));
      }
      break;
    }
    default: {
      ESP_LOGW(TAG, "[%s] Unknown operation: %s -> %d ['%s']", __func__, op_str, op, GET_OP_TYPE_NAME(op));
      cJSON_SetValuestring(status_obj, "error");
      cJSON_AddStringToObject(response, "message", "Unknown operation");
      result = ESP_FAIL;
      break;
    }
  }

  /* Send json to the thread */
  result = publishResponse(response, &msg);

  cJSON_Delete(response);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse json format payload
 *
 * @param json_str - json message
 *
 * {
 *    "operation": "set",
 *    "sensor": "name-of-sensor",
 *    "data": [ 
 *       "threshold": {
 *         "min": value,
 *         "max": value
 *       }
 *    ]
 * }
 *
 * {
 *   "operation": "get",
 *   "sensor": "name-of-sensor",
 *   "data": ["info", threshold", "lux", ...]
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
    const cJSON* sensor = cJSON_GetObjectItem(root, "sensor");
    const cJSON* data = cJSON_GetObjectItem(root, "data");
    if (operation && sensor && data) {
      const char* o_str = cJSON_GetStringValue(operation);
      const char* s_str = cJSON_GetStringValue(sensor);
      
      ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);
      ESP_LOGD(TAG, "[%s]    sensor: '%s'", __func__, s_str);

      result = useSensor(s_str, o_str, data);
    } else {
      if (!operation) {
        result = publishError("Bad format. Missing operation field.");
        ESP_LOGE(TAG, "[%s] Bad format. Missing operation field.", __func__);
      } else if (!sensor) {
        result = publishError("Bad format. Missing sensor field.");
        ESP_LOGE(TAG, "[%s] Bad format. Missing sensor field.", __func__);
      } else if (!data) {
        result = publishError("Bad format. Missing data field.");
        ESP_LOGE(TAG, "[%s] Bad format. Missing data field.", __func__);
      } else {
        result = publishError("Bad format");
      }
      ESP_LOGE(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(root));
    }
    cJSON_Delete(root);
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
      ESP_LOGW(TAG, "[%s] Unknown message type: %d", __func__, msg->type);
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
