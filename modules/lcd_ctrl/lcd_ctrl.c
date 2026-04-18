/**
 * @file lcd_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief LCD Controller
 * @version 0.1
 * @date 2025-02-23
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
#include "lcd_ctrl.h"

#include "lcd_hw.h"
#include "lcd_helper.h"

#include "err.h"
#include "lut.h"


#if !CONFIG_IDF_TARGET_ESP32 && !CONFIG_IDF_TARGET_ESP32S3
#error "lcd_ctrl supports only ESP32 and ESP32-S3 targets."
#endif


#define LCD_TASK_NAME             "lcd-task"
#define LCD_TASK_STACK_SIZE       4096
#define LCD_TASK_PRIORITY         10

#define LCD_MSG_MAX               8


static const char* TAG = "ESP::LCD";

/**
 * @brief Maps Ethernet application events to the ETH-connected flag on the LCD.
 *
 * @param event_id Value from the message bus (`data_eth_event_e`).
 */
static void lcdctrl_ApplyEthEvent(data_eth_event_e event_id) {
  bool connected = false;
  bool update = true;

  ESP_LOGI(TAG, "++%s(event_id: %d [%s])", __func__, event_id, GET_DATA_ETH_EVENT_NAME(event_id));
  switch (event_id) {
    case DATA_ETH_EVENT_CONNECTED: {
      connected = true;
      break;
    }
    case DATA_ETH_EVENT_DISCONNECTED: {
      connected = false;
      break;
    }
    case DATA_ETH_EVENT_STOP: {
      connected = false;
      break;
    }
    default: {
      update = false;
      break;
    }
  }
  if (update) {
    lcd_update_t u = {0};
    u.u.d_bool[0] = connected;
    lcd_UpdateData(LCD_MASK_ETH_CONNECTED, &u);
  }
  ESP_LOGI(TAG, "--%s() - update: %d, connected: %d", __func__, update, connected);
}

/**
 * @brief Maps Wi-Fi STA application events to the Wi-Fi-connected flag on the LCD.
 *
 * @param event_id Value from the message bus (`data_wifi_event_e`).
 */
static void lcdctrl_ApplyWifiEvent(data_wifi_event_e event_id) {
  bool connected = false;
  bool update = true;

  ESP_LOGI(TAG, "++%s(event_id: %d [%s])", __func__, event_id, GET_DATA_WIFI_EVENT_NAME(event_id));
  switch (event_id) {
    case DATA_WIFI_EVENT_CONNECTED: {
      connected = true;
      break;
    }
    case DATA_WIFI_EVENT_DISCONNECTED: {
      connected = false;
      break;
    }
    case DATA_WIFI_EVENT_STA_STOP: {
      connected = false;
      break;
    }
    default: {
      update = false;
      break;
    }
  }
  if (update) {
    lcd_update_t u = {0};
    u.u.d_bool[0] = connected;
    lcd_UpdateData(LCD_MASK_WIFI_CONNECTED, &u);
  }
  ESP_LOGI(TAG, "--%s() - update: %d, connected: %d", __func__, update, connected);
}

/**
 * @brief Maps MQTT application events to the MQTT-connected flag on the LCD.
 *
 * @param event_id Value from the message bus (`data_mqtt_event_e`).
 */
static void lcdctrl_ApplyMqttEvent(data_mqtt_event_e event_id) {
  bool connected = false;
  bool update = true;

  ESP_LOGI(TAG, "++%s(event_id: %d [%s])", __func__, event_id, GET_DATA_MQTT_EVENT_NAME(event_id));
  switch (event_id) {
    case DATA_MQTT_EVENT_CONNECTED: {
      connected = true;
      break;
    }
    case DATA_MQTT_EVENT_DISCONNECTED: {
      connected = false;
      break;
    }
    default: {
      update = false;
      break;
    }
  }
  if (update) {
    lcd_update_t u = {0};
    u.u.d_bool[0] = connected;
    lcd_UpdateData(LCD_MASK_MQTT_CONNECTED, &u);
  }
  ESP_LOGI(TAG, "--%s() - update: %d, connected: %d", __func__, update, connected);
}

static void lcdctrl_ApplyRelayData(const char* json_str) {
  bool have_heater = false;
  bool have_pump = false;
  bool heater_on = false;
  bool pump_on = false;

  if (json_str == NULL || json_str[0] == '\0') {
    return;
  }

  ESP_LOGI(TAG, "++%s(json_str: '%s')", __func__, json_str);

  cJSON* root = cJSON_Parse(json_str);
  if (root == NULL) {
    ESP_LOGW(TAG, "[%s] cJSON_Parse() failed", __func__);
    return;
  }

  const cJSON* relays = cJSON_GetObjectItem(root, "relays");
  if (cJSON_IsArray(relays)) {
    int relay_cnt = cJSON_GetArraySize(relays);
    for (int idx = 0; idx < relay_cnt; ++idx) {
      const cJSON* relay = cJSON_GetArrayItem(relays, idx);
      const cJSON* o_number = cJSON_GetObjectItem(relay, "number");
      const cJSON* o_state = cJSON_GetObjectItem(relay, "state");
      if (!cJSON_IsNumber(o_number) || !cJSON_IsString(o_state)) {
        continue;
      }

      int number = cJSON_GetNumberValue(o_number);
      const char* state = cJSON_GetStringValue(o_state);
      bool on = (state != NULL && strcmp(state, "on") == 0);

      if (number == 0) {
        heater_on = on;
        have_heater = true;
      } else if (number == 1) {
        pump_on = on;
        have_pump = true;
      }
    }
  }

  if (have_heater || have_pump) {
    lcd_update_t u = {0};
    uint32_t mask = 0;

    if (have_heater) {
      u.u.d_bool[0] = heater_on;
      mask |= LCD_MASK_RELAY_HEATER;
    }
    if (have_pump) {
      u.u.d_bool[(mask != 0U) ? 1 : 0] = pump_on;
      mask |= LCD_MASK_RELAY_PUMP;
    }
    lcd_UpdateData(mask, &u);
  }

  cJSON_Delete(root);
  ESP_LOGI(TAG, "--%s() - have_heater: %d, heater_on: %d, have_pump: %d, pump_on: %d",
           __func__, have_heater, heater_on, have_pump, pump_on);
}

static QueueHandle_t      lcd_msg_queue = NULL;
static TaskHandle_t       lcd_task_id = NULL;
static SemaphoreHandle_t  lcd_sem_id = NULL;


/**
 * @brief Parses incoming LCD controller message and dispatches action.
 *
 * @param msg Pointer to received message.
 * @return esp_err_t ESP_OK or task control code, ESP_FAIL for unknown message.
 */
static esp_err_t lcdctrl_ParseMsg(const msg_t* msg) {
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
      lcd_update_t u = {0};
      strncpy(u.u.d_string, msg->payload.mgr.uid, sizeof(u.u.d_string) - 1);
      u.u.d_string[sizeof(u.u.d_string) - 1] = '\0';
      lcd_UpdateData(LCD_MASK_BOARD_UID, &u);
      break;
    }
    case MSG_TYPE_ETH_MAC: {
      lcd_update_t u = {0};
      memcpy(u.u.d_uint8, msg->payload.eth.u.mac, 6);
      lcd_UpdateData(LCD_MASK_ETH_MAC, &u);
      break;
    }
    case MSG_TYPE_ETH_IP: {
      lcd_update_t u = {0};
      u.u.d_uint32[0] = msg->payload.eth.u.info.ip;
      u.u.d_uint32[1] = msg->payload.eth.u.info.mask;
      u.u.d_uint32[2] = msg->payload.eth.u.info.gw;
      lcd_UpdateData(LCD_MASK_ETH_IP | LCD_MASK_ETH_NETMASK | LCD_MASK_ETH_GW, &u);
      break;
    }

    case MSG_TYPE_ETH_EVENT: {
      lcdctrl_ApplyEthEvent(msg->payload.eth.u.event_id);
      break;
    }

    case MSG_TYPE_WIFI_IP: {
      lcd_update_t u = {0};
      u.u.d_uint32[0] = msg->payload.wifi.u.ip_info.ip;
      u.u.d_uint32[1] = msg->payload.wifi.u.ip_info.mask;
      u.u.d_uint32[2] = msg->payload.wifi.u.ip_info.gw;
      lcd_UpdateData(LCD_MASK_WIFI_IP | LCD_MASK_WIFI_NETMASK | LCD_MASK_WIFI_GW, &u);
      break;
    }

    case MSG_TYPE_WIFI_MAC: {
      lcd_update_t u = {0};
      memcpy(u.u.d_uint8, msg->payload.wifi.u.mac, 6);
      lcd_UpdateData(LCD_MASK_WIFI_MAC, &u);
      break;
    }

    case MSG_TYPE_WIFI_EVENT: {
      lcdctrl_ApplyWifiEvent(msg->payload.wifi.u.event_id);
      break;
    }

    case MSG_TYPE_MQTT_EVENT: {
      lcdctrl_ApplyMqttEvent(msg->payload.mqtt.u.event_id);
      break;
    }

    case MSG_TYPE_MQTT_DATA: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      if ((msg->from & REG_RELAY_CTRL) || (strstr(data_ptr->topic, "relay") != NULL)) {
        lcdctrl_ApplyRelayData(data_ptr->msg);
      }
      break;
    }

    case MSG_TYPE_LCD_DATA: {
      lcd_update_t u = {0};
      memcpy(u.u.d_uint32, msg->payload.lcd.d_uint32, sizeof(u.u.d_uint32));
      lcd_UpdateData(msg->payload.lcd.mask, &u);
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
 * @brief LCD controller FreeRTOS task: dequeue messages and dispatch via lcdctrl_ParseMsg().
 *
 * @param param Unused (task parameter).
 */
static void lcdctrl_TaskFn(void* param) {
  msg_t msg;

  bool loop = true;
  esp_err_t result;

  ESP_LOGI(TAG, "++%s()", __func__);
  memset(&msg, 0x00, sizeof(msg_t));
  while (loop) {
    ESP_LOGD(TAG, "[%s] Wait...", __func__);
    if(xQueueReceive(lcd_msg_queue, &msg, portMAX_DELAY) == pdTRUE) {
      ESP_LOGD(TAG, "[%s] Message arrived: type: %d [%s], from: 0x%08lx, to: 0x%08lx", __func__, 
          msg.type, GET_MSG_TYPE_NAME(msg.type),
          msg.from, msg.to);
      
      result = lcdctrl_ParseMsg(&msg);
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
  if (lcd_sem_id) {
    xSemaphoreGive(lcd_sem_id);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Sends message to LCD controller internal queue.
 *
 * @param msg Pointer to message to enqueue.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t lcdctrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (lcd_msg_queue == NULL) {
    ESP_LOGW(TAG, "[%s] skipped (LCD controller not initialized)", __func__);
    return ESP_ERR_INVALID_STATE;
  }
  if (xQueueSend(lcd_msg_queue, msg, (TickType_t) 0) != pdPASS) {
    ESP_LOGE(TAG, "[%s] Message error. type: %d, from: 0x%08lx, to: 0x%08lx", __func__, msg->type, msg->from, msg->to);
    result = ESP_FAIL;
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initializes LCD helper resources and controller task.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t lcdctrl_Init(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  result = lcd_InitHelper();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_InitHelper() result: %d.", __func__, result);
    return result;
  }

  /* Initialization message queue */
  lcd_msg_queue = xQueueCreate(LCD_MSG_MAX, sizeof(msg_t));
  if (lcd_msg_queue == NULL)
  {
    ESP_LOGE(TAG, "[%s] xQueueCreate() failed.", __func__);
    return ESP_FAIL;
  }

  lcd_sem_id = xSemaphoreCreateCounting(1, 0);
  if (lcd_sem_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xSemaphoreCreateCounting() failed.", __func__);
    return ESP_FAIL;
  }

  /* Initialization thread */
  xTaskCreate(lcdctrl_TaskFn, LCD_TASK_NAME, LCD_TASK_STACK_SIZE, NULL, LCD_TASK_PRIORITY, &lcd_task_id);
  if (lcd_task_id == NULL)
  {
    ESP_LOGE(TAG, "[%s] xTaskCreate() failed.", __func__);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Stops controller task and releases LCD helper resources.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t lcdctrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  if (lcd_sem_id) {
    msg_t msg = {
      .type = MSG_TYPE_DONE,
      .from = REG_LCD_CTRL,
      .to = REG_LCD_CTRL,
    };
    result = lcdctrl_Send(&msg);

    ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake to finish task...", __func__);
    xSemaphoreTake(lcd_sem_id, portMAX_DELAY);

    vSemaphoreDelete(lcd_sem_id);
    ESP_LOGD(TAG, "[%s] Semaphore deleted", __func__);

    ESP_LOGD(TAG, "[%s] Task stopped", __func__);
  }
  if (lcd_msg_queue) {
    vQueueDelete(lcd_msg_queue);
    ESP_LOGD(TAG, "[%s] Queue deleted", __func__);
  }

  result = lcd_DoneHelper();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "[%s] lcd_DoneHelper() result: %d.", __func__, result);
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Executes LCD controller runtime logic.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
static esp_err_t lcdctrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Initializes LCD controller public interface.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t LcdCtrl_Init(void) {
  esp_err_t result = ESP_OK;

  esp_log_level_set(TAG, CONFIG_LCD_CTRL_LOG_LEVEL);

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcdctrl_Init();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Deinitializes LCD controller public interface.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t LcdCtrl_Done(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcdctrl_Done();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Runs LCD controller public interface.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t LcdCtrl_Run(void) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcdctrl_Run();
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Sends message to LCD controller thread.
 *
 * @param msg Pointer to message to send.
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
esp_err_t LcdCtrl_Send(const msg_t* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);
  result = lcdctrl_Send(msg);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}
