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
#include "nvs_ctrl.h"
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

static data_uid_t         esp_uid = {0};

static char               mqtt_buffer[20];

typedef struct {
  char      uri[20];
  uint32_t  port;
} mqttctrl_address_t;

static mqttctrl_address_t mqtt_address;
static nvs_t mqtt_nvs_handle = NULL;


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

      ESP_LOGD(TAG, " SIZE: topic: %d, data: %d", event->topic_len, event->data_len);

      if (event->topic_len && (event->topic_len < DATA_TOPIC_SIZE)) {
        memcpy(msg.payload.mqtt.u.data.topic, event->topic, event->topic_len);
        msg.payload.mqtt.u.data.topic[event->topic_len] = 0;
      } else {
        ESP_LOGE(TAG, "[%s] Wrong topic size: %d", __func__, event->topic_len);
        break;
      }

      if (event->data_len && (event->data_len < DATA_MSG_SIZE)) {
        memcpy(msg.payload.mqtt.u.data.msg, event->data, event->data_len);
        msg.payload.mqtt.u.data.msg[event->data_len] = 0;

        /* remove all unnecessary characters */
        cJSON_Minify(msg.payload.mqtt.u.data.msg);

      } else {
        ESP_LOGE(TAG, "[%s] Wrong data size: %d", __func__, event->data_len);
        break;
      }

      ESP_LOGD(TAG, "TOPIC: [%3d] '%s'", strlen(msg.payload.mqtt.u.data.topic), msg.payload.mqtt.u.data.topic);
      ESP_LOGD(TAG, " DATA: [%3d] '%s'", strlen(msg.payload.mqtt.u.data.msg), msg.payload.mqtt.u.data.msg);

      send = true;
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

static esp_err_t mqttctrl_SetAddress(const cJSON* address) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(address: %p)", __func__, address);
  if (address) {
    char* uri = NULL;
    uint32_t port = 0;

    cJSON* obj = cJSON_GetObjectItem(address, "uri");
    if (cJSON_IsString(obj)) {
      uri = cJSON_GetStringValue(obj);
    }
    obj = cJSON_GetObjectItem(address, "port");
    if (cJSON_IsNumber(obj)) {
      port = (uint32_t) cJSON_GetNumberValue(obj);
    }

    if (uri) {
      memcpy(&mqtt_address.uri, uri, strlen(uri));
    }

    if ((port > 0) && (port <= 65535)) {
      mqtt_address.port = port;
    }

    ESP_LOGD(TAG, "[%s] uri: '%s', port: %ld", __func__, uri ? uri : "---", port);

    if (mqtt_nvs_handle) {
      NVS_Write(mqtt_nvs_handle, "address", &mqtt_address, sizeof(mqttctrl_address_t));
    }

  } else {
    ESP_LOGE(TAG, "[%s] Bad data format. Missing operation field.", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_SetBroker(const cJSON* broker) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(broker: %p)", __func__, broker);
  if (broker) {
    result = mqttctrl_SetAddress(cJSON_GetObjectItem(broker, "address"));
    ESP_LOGD(TAG, "[%s] broker: '%s'", __func__, cJSON_PrintUnformatted(broker));
  } else {
    ESP_LOGE(TAG, "[%s] Bad data format. Missing operation field.", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqttctrl_ParseMqttData(const char* json_str) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(json_str: '%s')", __func__, json_str);
  cJSON* root = cJSON_Parse(json_str);
  if (root == NULL) {
    ESP_LOGE(TAG, "[%s] Unknown root: '%s'", __func__, json_str);
    return ESP_FAIL;
  }

  const cJSON* operation = cJSON_GetObjectItem(root, "operation");
  if (operation) {
    const char* o_str = cJSON_GetStringValue(operation);
    ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);
    if (strcmp(o_str, "set") == 0) {
      result = mqttctrl_SetBroker(cJSON_GetObjectItem(root, "broker"));

    } else if (strcmp(o_str, "get") == 0) {

    } else {
      ESP_LOGW(TAG, "[%s] Unknown operation: '%s'", __func__, o_str);
    }
  } else {
    ESP_LOGE(TAG, "[%s] Bad data format. Missing operation field.", __func__);
    ESP_LOGE(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(root));
  }
  cJSON_Delete(root);
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

    case MSG_TYPE_MGR_UID: {
      memcpy(esp_uid, msg->payload.mgr.uid, strlen(msg->payload.mgr.uid) + 1);
      ESP_LOGD(TAG, "[%s] UID: '%s'", __func__, esp_uid);
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
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      ESP_LOGD(TAG, "[%s] topic: '%s'", __func__, data_ptr->topic);
      ESP_LOGD(TAG, "[%s]   msg: '%s'", __func__, data_ptr->msg);
      result = mqttctrl_ParseMqttData(data_ptr->msg);
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
      ESP_LOGW(TAG, "[%s] Unknown message type: %d", __func__, msg->type);
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

  memset(&mqtt_address, 0x00, sizeof(mqttctrl_address_t));

  result = NVS_Open("mqtt-ctrl", &mqtt_nvs_handle);
  if (result == ESP_OK) {
    size_t data_size = sizeof(mqttctrl_address_t);

    result = NVS_Read(mqtt_nvs_handle, "address", &mqtt_address, &data_size);
    if (result == ESP_OK) {
      ESP_LOGD(TAG, "[%s] NVS_Read() -> uri: '%s', port: %ld", __func__, mqtt_address.uri, mqtt_address.port);
    } else {
      ESP_LOGE(TAG, "[%s] NVS_Read() - result: %d.", __func__, result);
    }
  } else {
    ESP_LOGE(TAG, "[%s] Cannot open 'mqtt-ctrl' partition - result: %d.", __func__, result);
  }
  
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

  if (mqtt_nvs_handle) {
    NVS_Close(mqtt_nvs_handle);
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
