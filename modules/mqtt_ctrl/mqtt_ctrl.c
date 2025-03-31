/**
 * @file mqtt_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief MQTT Controller
 * @version 0.1
 * @date 2025-02-03
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "mqtt_client.h"

#include "sdkconfig.h"

#include "msg.h"
#include "mgr_ctrl.h"
#include "mqtt_ctrl.h"

#include "err.h"
#include "lut.h"
#include "mqtt_lut.h"


/* Temporary Broker URL - should be taken from NVM */
#define CONFIG_BROKER_URL       "mqtt://10.0.0.10"

#define MQTT_TASK_NAME          "mqtt-task"
#define MQTT_TASK_STACK_SIZE    4096
#define MQTT_TASK_PRIORITY      9

#define MQTT_MSG_MAX            10

#define MQTT_TOPIC_MAX_LEN      (32U)
#define MQTT_UID_LEN            (10U)
#define MQTT_TOPIC_LEN          (MQTT_TOPIC_MAX_LEN - MQTT_UID_LEN - 1)

#define MQTT_UID_IDX            (0U)
#define MQTT_TOPIC_IDX          (10U)


static const char* TAG = "ESP::MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;

static QueueHandle_t      mqtt_msg_queue = NULL;
static TaskHandle_t       mqtt_task_id = NULL;
static SemaphoreHandle_t  mqtt_sem_id = NULL;


/**
 * @brief MQTT event handler
 *
 * @param handler_args
 * @param base
 * @param event_id
 * @param event_data
 */
static void mqttctrl_EventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;
  msg_t msg;
  bool send = false;

  ESP_LOGI(TAG, "++%s(handler_args: %p, base: %s, event_id: %ld, event_data: %p)", __func__, handler_args, base ? base : "-", event_id, event_data);
  ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event->event_id, GET_MQTT_EVENT_NAME(event->event_id));

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED: {
      msg.type = MSG_TYPE_MQTT_EVENT;
      msg.from = REG_MQTT_CTRL;
      msg.to = REG_ALL_CTRL;
      msg.payload.mqtt.u.event_id = DATA_MQTT_EVENT_CONNECTED;
      send = true;
      break;
    }
    case MQTT_EVENT_DISCONNECTED: {
      break;
    }
    case MQTT_EVENT_SUBSCRIBED: {
      break;
    }
    case MQTT_EVENT_UNSUBSCRIBED: {
      esp_mqtt_client_disconnect(mqtt_client);
      break;
    }
    case MQTT_EVENT_PUBLISHED: {
      break;
    }
    case MQTT_EVENT_DATA: {
      msg.type = MSG_TYPE_MQTT_DATA;
      msg.from = REG_MQTT_CTRL;
      msg.to = REG_MGR_CTRL;

      ESP_LOGI(TAG, "TOPIC: [%3d] '%s'", event->topic_len, event->topic);
      ESP_LOGI(TAG, " DATA: [%3d] '%s'", event->data_len, event->data);

      /* Add NULL at the end of buffer */
      event->topic[event->topic_len] = 0;
      event->data[event->data_len] = 0;

      /* remove all unnecessary characters */
      cJSON_Minify(event->data);

      /* Set length with NULL */
      event->topic_len = strlen(event->topic);
      event->data_len = strlen(event->data);

      ESP_LOGI(TAG, "TOPIC: [%3d] '%s'", event->topic_len, event->topic);
      ESP_LOGI(TAG, " DATA: [%3d] '%s'", event->data_len, event->data);

      if ((event->topic_len < DATA_TOPIC_SIZE) && (event->data_len < DATA_MSG_SIZE)) {
        memcpy(msg.payload.mqtt.u.data.topic, event->topic, event->topic_len);
        memcpy(msg.payload.mqtt.u.data.msg, event->data, event->data_len);
        send = true;
      } else {
        ESP_LOGE(TAG, "[%s] Size is too big -> topic: %d, data: %d", __func__,
            event->topic_len, event->data_len);
      }
      break;
    }
    case MQTT_EVENT_BEFORE_CONNECT: {
      break;
    }
    case MQTT_EVENT_ERROR: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_ERROR", __func__);
      break;
    }
    default: {
      ESP_LOGD(TAG, "[%s] Unknown event_id: %d", __func__, event->event_id);
      break;
    }
  }
  if (send) {
    ESP_LOGD(TAG, "[%s] MGR_Send() -> msg.type: %d [%s]", __func__, 
        msg.type, GET_MSG_TYPE_NAME(msg.type));
    if (MGR_Send(&msg) != ESP_OK) {
      ESP_LOGE(TAG, "[%s] Message error. type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__,
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t mqttctrl_InitClient(void) {
  esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = CONFIG_BROKER_URL,
    .session.protocol_ver = MQTT_PROTOCOL_V_5,
    .network.disable_auto_reconnect = true,
    // .credentials.username = "123",
    // .credentials.authentication.password = "456",
    // .session.last_will.topic = "/topic/will",
    // .session.last_will.msg = "i will leave",
    // .session.last_will.msg_len = 12,
    // .session.last_will.qos = 1,
    // .session.last_will.retain = true,
  };
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (mqtt_client) {
    result = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqttctrl_EventHandler, NULL);
  } else {
    ESP_LOGE(TAG, "[%s] esp_mqtt_client_init() failed", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_DoneClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    result = esp_mqtt_client_unregister_event(mqtt_client, ESP_EVENT_ANY_ID, mqttctrl_EventHandler);
    ESP_LOGD(TAG, "[%s] esp_mqtt_client_unregister_event() - result: %d", __func__, result);
    result = esp_mqtt_client_stop(mqtt_client);
    ESP_LOGD(TAG, "[%s] esp_mqtt_client_stop() - result: %d", __func__, result);
    result = esp_mqtt_client_destroy(mqtt_client);
    ESP_LOGD(TAG, "[%s] esp_mqtt_client_destroy() - result: %d", __func__, result);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_StartClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    result = esp_mqtt_client_start(mqtt_client);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_StopClient(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    result = esp_mqtt_client_stop(mqtt_client);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_Publish(const char* topic, const char* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(topic: '%s', msg: '%s')", __func__, topic, msg);
  int msg_id = esp_mqtt_client_publish(mqtt_client, topic, msg, 0, 1, 0);
  ESP_LOGD(TAG, "[%s] PUBLISH(topic: '%s', msg: '%s') -> msg_id: %d", __func__,
      topic, msg, msg_id);
  if (msg_id == -1) {
    result = ESP_FAIL;
  } else if (msg_id == -2) {
    result = ESP_ERR_NO_MEM;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_SubscribeTopic(const char* topic) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(topic: '%s')", __func__, topic);
  int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
  ESP_LOGD(TAG, "[%s] SUBSCRIBE(topic: '%s') -> msg_id: %d", __func__, topic, msg_id);
  if (msg_id == -1) {
    result = ESP_FAIL;
  } else if (msg_id == -2) {
    result = ESP_ERR_NO_MEM;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_SubscribeList(const char* json_ptr) {
  cJSON *root;
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(json_ptr: '%s')", __func__, json_ptr);
  root = cJSON_Parse(json_ptr);
  if (root != NULL)
  {
    cJSON* list = cJSON_GetObjectItem(root, "topics");
    for (int idx = 0; idx < cJSON_GetArraySize(list); ++idx) {
      char* topic = cJSON_GetStringValue(cJSON_GetArrayItem(list, idx));
      if (topic) {
        ESP_LOGD(TAG, "[%s] topic[idx=%d]: '%s'", __func__, idx, topic);
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, 0);
        ESP_LOGD(TAG, "[%s] SUBSCRIBE(topic: '%s') -> msg_id: %d", __func__, topic, msg_id);
        if (msg_id == -1) {
          result = ESP_FAIL;
        } else if (msg_id == -2) {
          result = ESP_ERR_NO_MEM;
        }
        if (result != ESP_OK) {
          break;
        }
      }
    }
    cJSON_Delete(root);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_ParseMsg(const msg_t* msg) {
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
    case MSG_TYPE_MQTT_START: {
      result = mqttctrl_StartClient();
      if (result != ESP_OK) {
        ESP_LOGE(TAG, "[%s] mqttctrl_StartClient() - result: %d", __func__, result);
      }
      break;
    }
    case MSG_TYPE_MQTT_EVENT: {
      data_mqtt_event_e event_id = msg->payload.mqtt.u.event_id;

      ESP_LOGD(TAG, "[%s] event_id: %d [%s]", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
      break;
    }
    case MSG_TYPE_MQTT_DATA: {
      ESP_LOGD(TAG, "[%s] data", __func__);
      break;
    }
    case MSG_TYPE_MQTT_PUBLISH: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      result = mqttctrl_Publish(data_ptr->topic, data_ptr->msg);
      break;
    }
    case MSG_TYPE_MQTT_SUBSCRIBE: {
      result = mqttctrl_SubscribeTopic(msg->payload.mqtt.u.topic);
      break;
    }
    case MSG_TYPE_MQTT_SUBSCRIBE_LIST: {
      result = mqttctrl_SubscribeList(msg->payload.mqtt.u.json);
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
 * @brief MQTT task's function
 * 
 * @param param 
 */
static void mqttctrl_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(mqtt_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);

      result = mqttctrl_ParseMsg(&msg);
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
  if (mqtt_sem_id) {
    xSemaphoreGive(mqtt_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t mqttctrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(mqtt_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  mqtt_msg_queue = xQueueCreate(MQTT_MSG_MAX, sizeof(msg_t));
  if (mqtt_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  mqtt_sem_id = xSemaphoreCreateCounting(1, 0);
  if (mqtt_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization MQTT thread */
  xTaskCreate(mqttctrl_TaskFn, MQTT_TASK_NAME, MQTT_TASK_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, &mqtt_task_id);
  if (mqtt_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  result = mqttctrl_InitClient();
  if (result != ESP_OK)
  {
    ESP_LOGE(TAG, "[%s] mqttctrl_InitClient() - result: %d", __func__, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_DoneClient();
  if (mqtt_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_MQTT_CTRL,
      .to = REG_MQTT_CTRL,
    };
    result = mqttctrl_Send(&msg);
  
    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(mqtt_sem_id, portMAX_DELAY);

    vSemaphoreDelete(mqtt_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (mqtt_msg_queue) {
    vQueueDelete(mqtt_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Init MQTT controller
 * 
 * \return esp_err_t 
 */
esp_err_t MqttCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_MQTT_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Done MQTT controller
 * 
 * \return esp_err_t 
 */
esp_err_t MqttCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Run MQTT controller
 * 
 * \return esp_err_t 
 */
esp_err_t MqttCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Send message to the MQTT controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t MqttCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqttctrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
