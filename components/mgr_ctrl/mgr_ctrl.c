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

#include "cJSON.h"

#include "sdkconfig.h"

#include "tags.h"

#include "mgr_ctrl.h"
#include "mgr_reg.h"

#include "mgr_reg_list.h"

#include "lut.h"


#define MGR_TASK_NAME           "mgr-task"
#define MGR_TASK_STACK_SIZE     4096
#define MGR_TASK_PRIORITY       10

#define MGR_MSG_MAX             40

#define GET_ETH_MAC(_mac)       _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]

#define MGR_TOPIC_MAX_LEN       (32U)
#define MGR_UID_LEN             (10U)
#define MGR_TOPIC_LEN           (MGR_TOPIC_MAX_LEN - MGR_UID_LEN - 1)

#define MGR_UID_IDX             (0U)
#define MGR_TOPIC_IDX           (10U)


static const char* TAG = MGR_CTRL_TAG;


static int mgr_modules_cnt = MGR_REG_LIST_CNT;

static QueueHandle_t      mgr_msg_queue = NULL;
static TaskHandle_t       mgr_task_id = NULL;
static SemaphoreHandle_t  mgr_sem_id = NULL;

static data_eth_mac_t     mgr_eth_mac = {};
static data_eth_info_t    mgr_eth_info = {};

static data_json_t        mgr_module_list = "";


/**
 * @brief Buffer to prepare topic
 *
 * The buffer is used to prepare the topic.
 * It consists of the following parts:
 *
 *   UID/topic
 *      where:
 *        - UID - has 10 bytes: 'ESP_12AB34'
 *        - topic - has MQTT_TOPIC_LEN bytes: '/req/sys', /res/sys, /event/sys
 */
static char     mgr_topic_buffer[MGR_TOPIC_MAX_LEN];

static char     mgr_uid_pattern[] = "ESP_%02X%02X%02X";
static char     mgr_uid[MGR_UID_LEN + 1] = {}; /* keeps only UID, as: ESP_12AB34 */

static char*    mgr_uid_ptr    = &mgr_topic_buffer[MGR_UID_IDX];
static char*    mgr_topic_ptr  = &mgr_topic_buffer[MGR_TOPIC_IDX];

static mgr_reg_send_f mgr_send_to_mqtt_fn = NULL;



static esp_err_t mgr_Init(int id) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(id: %d, type: 0x%08lx)", __func__, id, mgr_reg_list[id].type);
  if (mgr_reg_list[id].init_fn) {
    result = mgr_reg_list[id].init_fn();
    if (mgr_reg_list[id].type & REG_MQTT_CTRL) {
      mgr_send_to_mqtt_fn = mgr_reg_list[id].send_fn;
    }
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

/**
 * @brief Create a UID
 *
 * Creates UID from MAC address.
 * It will be as: ESP_12AB34
 *
 */
static void mgr_CreateUid(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  sprintf(mgr_uid, mgr_uid_pattern, mgr_eth_mac[3], mgr_eth_mac[4], mgr_eth_mac[5]);
  sprintf(mgr_uid_ptr, mgr_uid_pattern, mgr_eth_mac[3], mgr_eth_mac[4], mgr_eth_mac[5]);

  ESP_LOGD(TAG, "[%s]     mgr_uid: '%s'", __func__, mgr_uid);
  ESP_LOGD(TAG, "[%s] mgr_uid_ptr: '%s'", __func__, mgr_uid_ptr);

  ESP_LOGI(TAG, "--%s()", __func__);
}

/**
 * @brief Create a list of registered modules
 * 
 */
void mgr_CreateModuleList(void) {
  const char json_reg[] = "{ \"uid\": \"\", \"list\": [] }";
  cJSON *root;

  ESP_LOGI(TAG, "++%s()", __func__);
  ESP_LOGD(TAG, "[%s] MAC: %02X:%02X:%02X:%02X:%02X:%02X", __func__, GET_ETH_MAC(mgr_eth_mac));
  root = cJSON_Parse(json_reg);
  if (root != NULL)
  {
    cJSON *uid = cJSON_GetObjectItem(root, "uid");
    cJSON_SetValuestring(uid, mgr_uid);

    cJSON *list = cJSON_GetObjectItem(root, "list");
    for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
      cJSON_AddItemToArray(list, cJSON_CreateString(mgr_reg_list[idx].name));
    }
    char* json_str = cJSON_PrintUnformatted(root);
    ESP_LOGD(TAG, "[%s] JSON: '%s'", __func__, json_str);
    if (mgr_send_to_mqtt_fn) {
      msg_t msg;

      msg.type = MSG_TYPE_MQTT_PUBLISH;
      msg.from = REG_MGR_CTRL;
      msg.to = REG_MQTT_CTRL;

      sprintf(msg.payload.mqtt.u.data.topic, "REGISTER/ESP");
      cJSON_PrintPreallocated(root, msg.payload.mqtt.u.data.msg, DATA_JSON_SIZE, 0);

      esp_err_t result = mgr_send_to_mqtt_fn(&msg);
    }
    cJSON_Delete(root);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

void mgr_CreateSubscribeList(void) {
  ESP_LOGI(TAG, "++%s()", __func__);

  ESP_LOGI(TAG, "--%s()", __func__);
}

void mgr_StartMqtt(void) {
  ESP_LOGI(TAG, "++%s()", __func__);
  if (mgr_send_to_mqtt_fn) {
    msg_t msg = {
      .type = MSG_TYPE_MQTT_START
    };
    esp_err_t result = mgr_send_to_mqtt_fn(&msg);
  }
  ESP_LOGI(TAG, "--%s()", __func__);
}

static esp_err_t mgr_ParseMqttData(const char* topic, const char* msg) {
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s(topic: '%s', msg: '%s')", __func__, topic, msg);

  /* Find msg_type_e from the topic */
  /* and call send_fn() for it      */


  ESP_LOGI(TAG, "--%s() - result: %d", __func__, result);
  return result;
}

static esp_err_t mgr_ParseMsg(const msg_t* msg) {
  esp_err_t result = ESP_FAIL;

  ESP_LOGI(TAG, "++%s(type: %d [%s], from: 0x%08lx, to: 0x%08lx)", __func__, 
      msg->type, GET_MSG_TYPE_NAME(msg->type),
      msg->from, msg->to);

  switch (msg->type) {

    case MSG_TYPE_ETH_EVENT: {
      data_eth_event_e event_id = msg->payload.eth.u.event_id;

      ESP_LOGD(TAG, "[%s] Event: %d [%s]", __func__, event_id, GET_DATA_ETH_EVENT_NAME(event_id));
      break;
    }

    case MSG_TYPE_ETH_MAC: {
      const data_eth_mac_t* mac_ptr = &(msg->payload.eth.u.mac);

      /* Store MAC address */
      memcpy(mgr_eth_mac, mac_ptr, sizeof(data_eth_mac_t));
      ESP_LOGD(TAG, "[%s] MAC: %02X:%02X:%02X:%02X:%02X:%02X", __func__, GET_ETH_MAC(mgr_eth_mac));

      mgr_CreateUid();
      mgr_CreateModuleList();
      mgr_CreateSubscribeList();
      break;
    }

    case MSG_TYPE_ETH_IP: {
      uint8_t* addr = NULL;

      const data_eth_info_t* info_ptr = &(msg->payload.eth.u.info);

      addr = (uint8_t*) &(info_ptr->ip);
      ESP_LOGD(TAG, "[%s]   IP: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      addr = (uint8_t*) &(info_ptr->mask);
      ESP_LOGD(TAG, "[%s] MASK: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      addr = (uint8_t*) &(info_ptr->gw);
      ESP_LOGD(TAG, "[%s]   GW: %d.%d.%d.%d", __func__, 
        addr[0], addr[1], addr[2], addr[3]
      );

      mgr_StartMqtt();
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
      result = mgr_ParseMqttData(data_ptr->topic, data_ptr->msg);
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
  esp_err_t result = ESP_OK;

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
        result = mgr_ParseMsg(&msg);
      }

      /* Now, notify specific (or all) registered controller */
      if (msg.to & (~REG_MGR_CTRL)) {
        result = mgr_NotifyCtrl(&msg);
      }

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
  esp_err_t result = ESP_OK;

  ESP_LOGI(TAG, "++%s()", __func__);

  for (int idx = 0; idx < mgr_modules_cnt; ++idx) {
    result = mgr_Run(idx);
  }

  ESP_LOGD(TAG, "[%s] Wait on xSemaphoreTake...", __func__);
  xSemaphoreTake(mgr_sem_id, portMAX_DELAY);
  ESP_LOGD(TAG, "[%s] xSemaphoreTake was released.", __func__);

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
