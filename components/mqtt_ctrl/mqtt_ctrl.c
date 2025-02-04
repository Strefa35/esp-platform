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

#include "mqtt_client.h"

#include "sdkconfig.h"

#include "tags.h"

#include "msg.h"
#include "mqtt_ctrl.h"


#define MQTT_MSG_MAX           20

#define MQTT_TASK_NAME         "mqtt-task"
#define MQTT_TASK_STACK_SIZE   4096
#define MQTT_TASK_PRIORITY     10

#define CONFIG_BROKER_URL     "mqtt://10.0.0.10"


static const char* TAG = MQTT_CTRL_TAG;

static QueueHandle_t      mqtt_msg_queue = NULL;
static TaskHandle_t       mqtt_task_id = NULL;

static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqttctrl_EventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;
  msg_t msg = { .type = MSG_TYPE_MQTT_EVENT, .from = MSG_MQTT_CTRL, .to = MSG_ALL_CTRL, .data.mqtt.event_id = event->event_id };
  bool send = true;

  ESP_LOGI(TAG, "++%s(handler_args: %p, base: %s, event_id: %ld, event_data: %p)", __func__, handler_args, base ? base : "-", event_id, event_data);

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_CONNECTED", __func__);
      break;
    }
    case MQTT_EVENT_DISCONNECTED: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_DISCONNECTED", __func__);
      break;
    }
    case MQTT_EVENT_SUBSCRIBED: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_SUBSCRIBED", __func__);
      break;
    }
    case MQTT_EVENT_UNSUBSCRIBED: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_UNSUBSCRIBED", __func__);
      esp_mqtt_client_disconnect(mqtt_client);
      break;
    }
    case MQTT_EVENT_PUBLISHED: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_PUBLISHED", __func__);
      break;
    }
    case MQTT_EVENT_DATA: {
      ESP_LOGD(TAG, "[%s] MQTT_EVENT_DATA", __func__);
      ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
      ESP_LOGI(TAG, " DATA: %.*s", event->data_len, event->data);
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
    if (xQueueSend(mqtt_msg_queue, &msg, (TickType_t) 0) != pdPASS) {
      ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg.type, msg.from, msg.to);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t mqtt_InitClient(void) {
  esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = CONFIG_BROKER_URL,
    .session.protocol_ver = MQTT_PROTOCOL_V_5,
    .network.disable_auto_reconnect = true,
    .credentials.username = "123",
    .credentials.authentication.password = "456",
    .session.last_will.topic = "/topic/will",
    .session.last_will.msg = "i will leave",
    .session.last_will.msg_len = 12,
    .session.last_will.qos = 1,
    .session.last_will.retain = true,
  };
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (mqtt_client) {
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqttctrl_EventHandler, NULL);
    esp_mqtt_client_start(mqtt_client);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqtt_DoneClient(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (mqtt_client) {
    esp_mqtt_client_unregister_event(mqtt_client, ESP_EVENT_ANY_ID, mqttctrl_EventHandler);
    esp_mqtt_client_stop(mqtt_client);
    esp_mqtt_client_destroy(mqtt_client);
  } else {
    ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mqtt_ParseEthMsg(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d, from: 0x%08lx, to: 0x%08lx)", __func__, msg->type, msg->from, msg->to);
  switch (msg->type) {
    case MSG_TYPE_ETH_EVENT: {
      ESP_LOGD(TAG, "[%s] Event: %s", __func__, msg->data.eth.event);
      break;
    }
    case MSG_TYPE_ETH_IP: {
      ESP_LOGD(TAG, "[%s]   IP: %s", __func__, msg->data.eth.ip);
      ESP_LOGD(TAG, "[%s] MASK: %s", __func__, msg->data.eth.mask);
      ESP_LOGD(TAG, "[%s]   GW: %s", __func__, msg->data.eth.gw);
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

static esp_err_t mqtt_ParseMqttMsg(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(type: %d, from: 0x%08lx, to: 0x%08lx)", __func__, msg->type, msg->from, msg->to);
  switch (msg->type) {
    case MSG_TYPE_MQTT_EVENT: {
      ESP_LOGD(TAG, "[%s] Event: %ld", __func__, msg->data.mqtt.event_id);
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

static esp_err_t mqtt_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(type: %d, from: 0x%08lx, to: 0x%08lx)", __func__, msg->type, msg->from, msg->to);
  switch (msg->from) {
    case MSG_ETH_CTRL: {
      result = mqtt_ParseEthMsg(msg);
      break;
    }
    case MSG_CLI_CTRL: {
      break;
    }
    case MSG_GPIO_CTRL: {
      break;
    }
    case MSG_POWER_CTRL: {
      break;
    }
    case MSG_MQTT_CTRL: {
      result = mqtt_ParseMqttMsg(msg);
      break;
    }
    default: {
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
static void mqtt_TaskFn(void* param) {
  msg_t msg;
  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(mqtt_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg.type, msg.from, msg.to);
      
      result = mqtt_ParseMsg(&msg);
      if (result != ESP_OK) {
        // TODO - Send Error to the Broker
        ESP_LOGE(TAG, "[%s] Error: %d", __func__, result);
      }
    } else {
      ESP_LOGE(TAG, "[%s] Message error.", __func__);
    }
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

esp_err_t mqttctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  /* Initialization message queue */
  mqtt_msg_queue = xQueueCreate(MQTT_MSG_MAX, sizeof(msg_t));
  if (mqtt_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization manager thread */
  xTaskCreate(mqtt_TaskFn, MQTT_TASK_NAME, MQTT_TASK_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, &mqtt_task_id);
  if (mqtt_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  result = mqtt_InitClient();
  if (result != ESP_OK)
  {
    ESP_LOGE(TAG, "[%s] mqtt_InitClient() - result: %d", __func__, result);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t mqttctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = mqtt_DoneClient();
  if (mqtt_task_id) {
    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (mqtt_msg_queue) {
    vQueueDelete(mqtt_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

esp_err_t mqttctrl_Run(void) {
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
 * @brief Sent message to the MQTT controller thread
 * 
 * \return esp_err_t 
 */
esp_err_t MqttCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (xQueueSend(mqtt_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
