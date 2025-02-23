/**
 * @file relay_ctrl.c
 * @author A.Czerwinski@pistacje.net
 * @brief Relay Controller
 * @version 0.1
 * @date 2025-02-19
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

#include "driver/gpio.h"

#include "tags.h"

#include "msg.h"
#include "relay_ctrl.h"

#include "err.h"
#include "lut.h"


#define RELAY_TASK_NAME           "relay-task"
#define RELAY_TASK_STACK_SIZE     4096
#define RELAY_TASK_PRIORITY       10

#define RELAY_MSG_MAX             10

#define RELAY_NUMBER_MIN          0
#define RELAY_NUMBER_MAX          1

#define RELAY_0                   (GPIO_NUM_32)
#define RELAY_1                   (GPIO_NUM_33)

#define GPIO_OUTPUT_PIN_SEL       ((1ULL << RELAY_0) | (1ULL << RELAY_1))

static const char* TAG = RELAY_CTRL_TAG;


static QueueHandle_t      relay_msg_queue = NULL;
static TaskHandle_t       relay_task_id = NULL;
static SemaphoreHandle_t  relay_sem_id = NULL;

static esp_err_t relayctrl_Configure(void) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);

  gpio_config_t gpio;

  gpio.intr_type = GPIO_INTR_DISABLE;
  gpio.mode = GPIO_MODE_OUTPUT;
  gpio.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  gpio.pull_down_en = 0;
  gpio.pull_up_en = 0;
  
  gpio_config(&gpio);

  gpio_set_level(RELAY_0, 0);
  gpio_set_level(RELAY_1, 0);

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t relayctrl_SetRelayState(const int number, const uint32_t level) {
  gpio_num_t gpio_num = (number == 0 ? RELAY_0 : RELAY_1);
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(number: %d, level: %ld)", __func__, number, level);
  result = gpio_set_level(gpio_num, level);
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t relayctrl_SetRelay(const cJSON* relay) {
  const cJSON* o_number = cJSON_GetObjectItem(relay, "number");
  const cJSON* o_state = cJSON_GetObjectItem(relay, "state");
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  
  if (cJSON_IsNumber(o_number) == false) {
    ESP_LOGE(TAG, "[%s] Invalid 'number'", __func__);
    return ESP_ERR_INVALID_ARG;
  }
  if (cJSON_IsString(o_state) == false) {
    ESP_LOGE(TAG, "[%s] Invalid 'state'", __func__);
    return ESP_ERR_INVALID_ARG;
  }

  const int number = cJSON_GetNumberValue(o_number);
  const char* state = cJSON_GetStringValue(o_state);

  if ((number < RELAY_NUMBER_MIN) || (number > RELAY_NUMBER_MAX)) {
    ESP_LOGE(TAG, "[%s] Relay number is out of range: %d", __func__, number);
    return ESP_FAIL;
  }

  if (strcmp(state, "on") == 0) {
    result = relayctrl_SetRelayState(number, 1);
  } else if (strcmp(state, "off") == 0){
    result = relayctrl_SetRelayState(number, 0);
  } else {
    ESP_LOGE(TAG, "[%s] Relay state is incorrect: %s", __func__, state);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t relayctrl_ParseRelays(const cJSON* relays) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s()", __func__);
  int relay_cnt = cJSON_GetArraySize(relays);
  if (relay_cnt) {
    for (int idx = 0; idx < relay_cnt; ++idx) {
      const cJSON* relay = cJSON_GetArrayItem(relays, idx);
      if (relay) {
        result = relayctrl_SetRelay(relay);
      } else {
        ESP_LOGE(TAG, "[%s] Bad relay format.", __func__);
        ESP_LOGE(TAG, "[%s] '%s'", __func__, cJSON_PrintUnformatted(relay));
        break;
      }
    }
  }
  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

/**
 * @brief Parse json format payload
 * 
 * @param json_str - json message
 * 
 * {
 *   "operation": "set/get"
 *   "relays": [
 *      { 
 *        "number": 0 or 1,
 *        "state": "on/off"
 *      },
 *   ]
 * 
 * }
 * 
 * @return esp_err_t 
 */
static esp_err_t relayctrl_ParseMqttData(const char* json_str) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(json_str: '%s')", __func__, json_str);
  cJSON* root = cJSON_Parse(json_str);
  if (root) {
    const cJSON* operation = cJSON_GetObjectItem(root, "operation");
    if (operation) {
      const char* o_str = cJSON_GetStringValue(operation);
      ESP_LOGD(TAG, "[%s] operation: '%s'", __func__, o_str);
      if (strcmp(o_str, "set") == 0) {
        const cJSON* relays = cJSON_GetObjectItem(root, "relays");

        result = relayctrl_ParseRelays(relays);

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

    case MSG_TYPE_MQTT_DATA: {
      const data_mqtt_data_t* data_ptr = &(msg->payload.mqtt.u.data);

      ESP_LOGD(TAG, "[%s] topic: '%s'", __func__, data_ptr->topic);
      ESP_LOGD(TAG, "[%s]   msg: '%s'", __func__, data_ptr->msg);
      result = relayctrl_ParseMqttData(data_ptr->msg);
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

  result = relayctrl_Configure();

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
